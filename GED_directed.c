// gcc GED_directed.c -o GED_directed -lm -O3

// Differences with GED.c are commented with 'DIRECTED' indicator

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

// **************************************************
// First a few functions
// **************************************************

// Euclidean distance
double dist(int i, int j, int dim, double **embed) {
  int k;
  double f = 0.0;
  for(k=0;k<dim;k++)
    f+=pow(embed[i][k]-embed[j][k],2.0);
  return(sqrt(f)); 
}

// Jensen-Shannon divergence with Dirichlet-like prior to handle 0's
// vC, vB : C-vector and B-vector we are comparing
// vI: indicator 1=internal, 0=external w.r.t. communities
// internal: if 1 return internal, if 0 return external JS distance
// if vI == internal == NULL, compute overall JS distance (ignore internal/external)
// vLen: length of the above
double JS(double *vC, double *vB, int *vI, int internal, int vLen) {
  double f,*vect_p,*vect_q,*vect_m;
  int i;

  vect_p = malloc(sizeof(double)*vLen);
  vect_q = malloc(sizeof(double)*vLen);
  vect_m = malloc(sizeof(double)*vLen);

  f = 0.0;
  if(vI!=NULL) {
    for(i=0;i<vLen;i++) if(vI[i]==internal) f+=(vC[i]+1);
    for(i=0;i<vLen;i++) if(vI[i]==internal) vect_p[i]=(vC[i]+1)/f;
  }
  else {
    for(i=0;i<vLen;i++) f+=(vC[i]+1);
    for(i=0;i<vLen;i++) vect_p[i]=(vC[i]+1)/f;  
  }  

  f = 0.0;
  if(vI!=NULL) {
    for(i=0;i<vLen;i++) if(vI[i]==internal) f+=(vB[i]+1);
    for(i=0;i<vLen;i++) if(vI[i]==internal) vect_q[i]=(vB[i]+1)/f;
    for(i=0;i<vLen;i++) if(vI[i]==internal) vect_m[i]=(vect_p[i]+vect_q[i])/2.0;
  }
  else {
    for(i=0;i<vLen;i++) f+=(vB[i]+1);
    for(i=0;i<vLen;i++) vect_q[i]=(vB[i]+1)/f;
    for(i=0;i<vLen;i++) vect_m[i]=(vect_p[i]+vect_q[i])/2.0;
  }

  f = 0.0;
  if(vI!=NULL) {
    for(i=0;i<vLen;i++) {
      if(vI[i]==internal) f+=vect_p[i]*log(vect_p[i]/vect_m[i])+vect_q[i]*log(vect_q[i]/vect_m[i]);
    }
  }
  else {
    for(i=0;i<vLen;i++) {
      f+=vect_p[i]*log(vect_p[i]/vect_m[i])+vect_q[i]*log(vect_q[i]/vect_m[i]); 
    }
  }  	
  f = f/2.0;

  free(vect_p);
  free(vect_q);
  free(vect_m);
  return(f);
}

// DIRECTED -- length is now n(n-1), so mapping from (i,j) to k differs.
// Order of P(i,j)'s is: (0,1) ... (0,n-1),(1,0),(1,2), ...
double myProb(double* P, int n, int i, int j) {
  return(P[(n-1)*i+j-(int)(j>i)]);
}

// ******************************************************************************

int main(int argc, char *argv[]) {

  int a, b, i, j, k, l, m, n_edge, v_min, v_max, c_min, c_max, n, vect_len, n_parts, dim, p_len;
  int emb_dim, emb_param;
  double f, lo, hi, best_div, best_alpha, alpha, diff, move, sm;
  FILE *fp, *fpa;
  int **edge, *comm, *deg_in, *deg_out;
  // DIRECTED -- from T and S to Tin, Tout, Sin, Sout
  double **embed, *P, *D, *Dst, *Tin, *Tout, *Sin, *Sout, *vect_C, *vect_B, *vect_p, *vect_q, *vect_m, *Q; 
  char str[256], *fn_edges, *fn_comm, *fn_embed;
  int opt, verbose=0, *vect_I, entropy=0, jsd_split=0, no_improvement=0, compute_proba=0;  
  double epsilon=0.25, delta=0.001, AlphaMax=10.0, AlphaStep=0.5; // default parameter values
  
  // randomized start
#ifdef WIN32
  srand((long)time(NULL));
#else
  srand48((long)time(NULL));
#endif

  // read in edgelist and communities filenames
  while ((opt = getopt(argc, argv, "g:c:e:d:vESPa:s:")) != -1) {
    switch (opt) {
    case 'a':
      AlphaMax = atof(optarg);
      break;
    case 'e':
      fn_embed = optarg;
      break;
    case 'g':
      fn_edges = optarg;
      break;
    case 'c':
      fn_comm = optarg;
      break;
    case 's':
      epsilon = atof(optarg);
      break;
    case 'd':
      delta = atof(optarg);
      break;
    case 'v':
      verbose=1;
      break;
    case 'E': // 'hidden' option, will return list of probas and entropy for anomaly detection
      entropy=1;
      break;
    case 'S': // split JS divergence in two parts
      jsd_split=1;
      break;
    case 'P': // output probabilities
      compute_proba=1;
      break;
    default: /* '?' */
      printf("Usage: %s -g graph_edgelist -c communities -e embedding [-a alpha_max -s epsilon_step -d delta -v -E]\n\n",argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  // Check min conditions
  if(fn_embed==NULL || fn_comm==NULL || fn_edges==NULL) {
      printf("Usage: %s -g graph_edgelist -c communities -e embedding [-a alpha_max -s epsilon_step -d delta -v]\n\n",argv[0]);
    exit(EXIT_FAILURE);
  }

  // 1.1 Read in the edgelist
  fp = fopen(fn_edges,"r");
  if(fp==NULL) {
    printf("Failed to open %s\n",fn_edges);
    exit(-1);
  }
  n_edge = 0;

  // count number of edges and keep track of min/max vertex
  v_max = -1;
  v_min = INT_MAX;
  while (fscanf(fp,"%d %d",&a,&b) == 2) {
    n_edge++;
    v_max = max(v_max,max(a,b));
    v_min = min(v_min,min(a,b));
  }
  rewind(fp);
  if(v_min<0 || v_min>1) {
    printf("Vertices should be either 0-based or 1-based\n");
    exit(EXIT_FAILURE);
  }
  edge = malloc(sizeof(int*) * n_edge);
  for(i=0; i<n_edge; i++)
    edge[i] = malloc(sizeof(int)*2);
  i = 0;
  // no weight, just 2 columns
  while (fscanf(fp,"%d %d",&a,&b) == 2) {
    edge[i][0] = a-v_min;
    edge[i++][1] = b-v_min;
  }
  fclose(fp);
  n = v_max-v_min+1;
  if(verbose) printf("Vertex range mapped from [%d,%d] to [%d,%d].\n",v_min,v_max,0,n-1);

  // 1.2 Read in the communities
  fp = fopen(fn_comm,"r");
  if(fp==NULL) {
    printf("Failed to open %s\n",fn_comm);
    exit(-1);
  }
  comm = malloc(sizeof(int) * (n));
  c_max = -1;
  c_min = INT_MAX;
  for(i=0; i<n; i++) {
    if (fscanf(fp, "%d",&a) == 1) {
      comm[i] = a;
      c_max = max(c_max,a);
      c_min = min(c_min,a);
    }
    else {
      printf("File of communities too short!\n");
      exit(-1);
    }
  }
  fclose(fp);
  if(c_min>0) {
    for(i=0; i<n; i++) comm[i]-=c_min;
    c_max -= c_min;
    c_min = 0;
  }
  if(verbose) printf("Communities mapped to [%d,%d].\n",c_min,c_max); 

  // 1.3 Compute degrees
  // DIRECTED -- need deg_in and deg_out instead of just degrees (recall that some can now be zero)
  deg_in = malloc(sizeof(int)*n);
  deg_out = malloc(sizeof(int)*n);    
  for(i=0; i<n; i++) {
    deg_in[i]=0;
    deg_out[i]=0;
  }
  for(i=0; i<n_edge; i++) {
    deg_out[edge[i][0]]++;
    deg_in[edge[i][1]]++;
  }

  // 1.4 Compute C-vector (and allocate all vectors while we are here)
  // DIRECTED -- longer vectors since we consdier all ORDERED pairs of parts
  n_parts = c_max+1;
  vect_len = n_parts*(n_parts);
  vect_C = malloc(sizeof(double)*vect_len);
  vect_B = malloc(sizeof(double)*vect_len);

  for(i=0;i<vect_len;i++) vect_C[i]=0.0;
  for(i=0;i<n_edge;i++) {
    // DIRECTED -- we use order: (0,0),(0,1) ... (0,n_parts-1),(1,0),(1,1), ...
    j = comm[edge[i][0]];
    k = comm[edge[i][1]];
    l = (j*n_parts) + k;
    vect_C[l]+=1.0;
  }
  
  // indicator - internal to a community?
  vect_I = malloc(sizeof(int)*vect_len);
  for(i=0;i<vect_len;i++) vect_I[i]=0;
  j=0;
  for(i=0;i<n_parts;i++) {
    vect_I[j]=1;
    j+=(n_parts+1);
  }

  // allocate memory for next steps
  // DIRECTED -- replace T, S with 2 copies: Tin and Tout, Sin and Sout
  Tin = malloc(sizeof(double)*n);
  Sin = malloc(sizeof(double)*n);
  Tout = malloc(sizeof(double)*n);
  Sout = malloc(sizeof(double)*n);
  // DIRECTED -- D remains with same length since Dij == Dji
  p_len = (n-1)*(n)/2;
  D = malloc(sizeof(double)*p_len);
  Dst = malloc(sizeof(double)*p_len);
  // DIRECTED -- P needs twice the length 
  P = malloc(sizeof(double)*(2*p_len));
  
  // 2. Read embedding in node2vec format:
  //    first line has number_vertices and dimensions
  //    other lines have vertex followed by embedding
  best_div = FLT_MAX;
  best_alpha = -1.0;

  fp = fopen(fn_embed,"r");
  if(fp==NULL) {
    printf("Failed to open %s\n",fn_embed);
    exit(-1);
  }
  fscanf(fp,"%d %d",&j,&dim);
  if(j!=n) {
    printf("Embedding has different number of vertices!\n");
    exit(-1);
  }
      
  embed = malloc(sizeof(double*)*n);
  for(i=0;i<n;i++)
    embed[i] = malloc(sizeof(double)*dim);
  for(i=0;i<n;i++) {
    if (fscanf(fp,"%d",&k)!=1) {
      printf("Error reading embedding file.\n");
      exit(-1);
    }
    for(j=0;j<dim;j++) {
      if(fscanf(fp,"%lf",&f)==1) {
	embed[k-v_min][j]=f;
      }
      else {
	printf("Error reading embedding file.\n");
	exit(-1);
      }
    }
  }
  fclose(fp);
  if(verbose) printf("Embedding has %d dimensions\n",dim);

  // 3.0 set up loop over alpha's      
  // Initialize Tin/Tout to 0 if degree is 0 in which case we get Sin or Sout == 0 so we should NOT update Tin or Tout to avoid div by 0  
  for(i=0;i<n;i++) {
    if(deg_in[i]==0) {
      Tin[i]=0.0;
    }
    else {
      Tin[i]=1.0;
    }
    if(deg_out[i]==0) {
      Tout[i]=0.0;
    }
    else {
      Tout[i]=1.0;
    }
  }

  // 3.1 compute Euclidean distance vector D[] given embed[][] and alpha
  // DIRECTED -- Dij == Dji so no change here
  lo = 10000; hi=0;
  for(i=0;i<(n-1);i++) {
    for(j=i+1;j<n;j++) {	
      f = dist(i,j,dim,embed); 
      if(f<lo) lo=f;
      if(f>hi) hi=f;
      k = n*i-i*(i+1)/2+j-i-1;
      Dst[k] = f; 
    }
  }
  
  for(alpha=AlphaStep; alpha<=AlphaMax+delta; alpha+=AlphaStep) {
    
    // apply kernel
    for(i=0;i<(n-1);i++) {
      for(j=i+1;j<n;j++) {	
	k = n*i-i*(i+1)/2+j-i-1;
	D[k] = (Dst[k]-lo)/(hi-lo); // normalize to [0,1]
	D[k] = pow(1-D[k],alpha); // transform w.r.t. alpha
      }
    }
    
    // 3.2 Learn GCL model numerically

    // 3.2.1 LOOP: compute T[] given: degree[] D[] epsilon delta
    // DIRECTED -- use Tin, Tout, deg_in, deg_out, Sin, Sout
    // for D, recall that Dij == Dji; we stored Dij for all i<j
    
    diff = 1.0;
    while(diff > delta) { // stopping criterion
      for(i=0;i<n;i++) {
	Sin[i]=0.0;
	Sout[i]=0.0;
      }
      k = 0;
      // DIRECTED -- loop over all i<j and do BOTH orders (i,j) and (j,i)
      for(i=0;i<(n-1);i++) {
	for(j=i+1;j<n;j++) {
	  Sin[i] += (Tin[i]*Tout[j]*D[k]);
	  Sin[j] += (Tin[j]*Tout[i]*D[k]);
	  Sout[i] += (Tout[i]*Tin[j]*D[k]);
	  Sout[j] += (Tout[j]*Tin[i]*D[k]);
	  k++;
	}
      }
      f = 0.0;

      // DIRECTED -- do not apply move if degree is zero
      for(i=0;i<n;i++) {
	if(deg_in[i]>0){
	  move = epsilon*Tin[i]*(deg_in[i]/Sin[i]-1.0); 
	  Tin[i]+=move;
	  f = max(f,fabs(deg_in[i]-Sin[i])); // convergence w.r.t. degrees
	}
	if(deg_out[i]>0){
	  move = epsilon*Tout[i]*(deg_out[i]/Sout[i]-1.0); 
	  Tout[i]+=move;
	  f = max(f,fabs(deg_out[i]-Sout[i])); // convergence w.r.t. degrees
	}
      }
      diff = f;
    }

    // 3.2.2 Compute probas P[]
    // DIRECTED -- not only for i<j, do all pairs i!=j
    k = (int)100*alpha;
    sprintf(str,"_proba_%d",k);
    if(compute_proba==1)
      fp = fopen(str,"w");
    for(i=0;i<n;i++) {
      for(j=0;j<n;j++) {
	if(i!=j) {
	  k = (n-1)*i+j-(int)(j>i);
	  a = min(i,j);
	  b = max(i,j);
	  l = n*a-a*(a+1)/2+b-a-1;
	  P[k] = (Tin[i]*Tout[j]*D[l]);
	  // OUTPUT: i->j, out_degree_i, in_degree_j, edge probability 
	  if(compute_proba==1)
	    fprintf(fp,"%d %d %d %d %f %d %d\n",i,j,deg_out[i],deg_in[j],P[k],comm[i],comm[j]);
	}
	
      }
    }
    if(compute_proba==1)
      fclose(fp);
    
    // 3.3 Compute B-vector given P[] and comm[]
    // DIRECTED -- again consider both i<j, i>j
    for(i=0;i<vect_len;i++) vect_B[i] = 0.0;
    for(i=0;i<n;i++) {
      for(j=0;j<n;j++) {
	if(i!=j) {
	  m = comm[i]*n_parts + comm[j];
	  vect_B[m] += myProb(P,n,i,j);
	}
      }
    }
    
    // 3.4 JS score -- keep best score
    if(jsd_split)
      f = (JS(vect_C, vect_B, vect_I, 1, vect_len) + JS(vect_C, vect_B, vect_I, 0, vect_len))/2.0;
    else
      f = JS(vect_C, vect_B, NULL, -1, vect_len);
    if(f < best_div) {
      best_div = f;
      best_alpha = alpha;
      no_improvement=0;
      if(verbose) {
	for(i=0;i<vect_len;i++) printf("%.4f ",vect_C[i]);
	printf("\n");
	for(i=0;i<vect_len;i++) printf("%.4f ",vect_B[i]);
	printf("\n");
      }
	
    }
    else
      no_improvement++;
    
    // break early if no improvement for 3 steps
    if(no_improvement>2)
      if(compute_proba==0)
	break;       
  }
  
  if(verbose) {
 	for(i=0;i<vect_len;i++) printf("%d ",vect_I[i]);
	printf("\n");
      }
   
  printf("Divergence: %e\n",best_div);

 
  // ************************************************************************
  // hidden -- get _probas (for debug) and _entropy (anomaly detection)
  // ************************************************************************
  if(entropy) {
    // REPEAT 3.0 with best_alpha (cut and paste from above code)  
    alpha = best_alpha;
    
    // apply kernel
    for(i=0;i<(n-1);i++) {
      for(j=i+1;j<n;j++) {	
	k = n*i-i*(i+1)/2+j-i-1;
	D[k] = (Dst[k]-lo)/(hi-lo); // normalize to [0,1]
	D[k] = pow(1-D[k],alpha); // transform w.r.t. alpha
      }
    }
        
    // 3.2 Learn GCL model numerically
    
    // 3.2.1 LOOP: 
    for(i=0;i<n;i++) {
      if(deg_in[i]==0) {
	Tin[i]=0.0;
      }
      else {
	Tin[i]=1.0;
      }
      if(deg_out[i]==0) {
	Tout[i]=0.0;
      }
      else {
	Tout[i]=1.0;
      }
    }
    diff = 1.0;
    while(diff > delta) { // stopping criterion
      for(i=0;i<n;i++) {
	Sin[i]=0.0;
	Sout[i]=0.0;
      }
      k = 0;
      for(i=0;i<(n-1);i++) {
	for(j=i+1;j<n;j++) {
	  Sin[i] += (Tin[i]*Tout[j]*D[k]);
	  Sin[j] += (Tin[j]*Tout[i]*D[k]);
	  Sout[i] += (Tout[i]*Tin[j]*D[k]);
	  Sout[j] += (Tout[j]*Tin[i]*D[k]);
	  k++;
	}
      }
      f = 0.0;
      
      for(i=0;i<n;i++) {
	if(deg_in[i]>0){
	  move = epsilon*Tin[i]*(deg_in[i]/Sin[i]-1.0); 
	  Tin[i]+=move;
	  f = max(f,fabs(deg_in[i]-Sin[i])); // convergence w.r.t. degrees
	}
	if(deg_out[i]>0){
	  move = epsilon*Tout[i]*(deg_out[i]/Sout[i]-1.0); 
	  Tout[i]+=move;
	  f = max(f,fabs(deg_out[i]-Sout[i])); // convergence w.r.t. degrees
	}
      }
      diff = f;
    }
    
    // 3.2.2 Compute probas P[]
    fp = fopen("_probas","w");
    for(i=0;i<n;i++) {
      for(j=0;j<n;j++) {
	if(i!=j) {
	  k = (n-1)*i+j-(int)(j>i);
	  a = min(i,j);
	  b = max(i,j);
	  l = n*a-a*(a+1)/2+b-a-1; // index for D
	  P[k] = (Tin[i]*Tout[j]*D[l]);
	  // OUTPUT: i->j, out_degree_i, in_degree_j, edge probability 
	  fprintf(fp,"%d %d %d %d %f\n",i,j,deg_out[i],deg_in[j],P[k]);
	}
      }
    }
    fclose(fp);
    
    // compute community probability sums for each node in turn + entropy
    // OUTPUT: node, entropy, probabilities toward each community (normalized sums)
    fp = fopen("_entropy","w");
    if(fp==NULL) {
      printf("Failed to open _entropy\n");
      exit(-1);
    }
    for(i=0;i<n;i++) {
      Q = (double *)malloc(n_parts*sizeof(double));
      for(j=0;j<n_parts;j++) Q[j]=0;
      for(j=0;j<n;j++)
	if(i!=j){
	  // consider both directions
	  Q[comm[j]] += myProb(P,n,i,j);
	  Q[comm[j]] += myProb(P,n,j,i);
	}
      fprintf(fp,"%d",i);
      // normlize probas
      sm = 0;
      for(j=0;j<n_parts;j++)
	sm += Q[j];
      for(j=0;j<n_parts;j++)
	Q[j] /= sm;
      // entropy
      sm = 0;
      for(j=0;j<n_parts;j++)
	sm = sm - Q[j]*log(Q[j]);
      // print
      fprintf(fp,",%f",sm);
      for(j=0;j<n_parts;j++)
	fprintf(fp,",%f",Q[j]);
      fprintf(fp,"\n");
    }
    fclose(fp);
    free(Q);
  }

  // 4. clean all
  for(i=0;i<n;i++) free(embed[i]);
  free(embed);
  free(vect_C);
  free(vect_B);
  free(Tin);
  free(Tout);
  free(Sin);
  free(Sout);
  free(deg_in);
  free(deg_out);
  free(D);
  free(Dst);
  free(P);
  // fprintf(stdout,"%lf %e",best_alpha,best_div);
  if(verbose) {
      printf("Best value for alpha: %lf\n",best_alpha);
  }

  return(0);
}
