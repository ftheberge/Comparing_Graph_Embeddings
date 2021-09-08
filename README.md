# Scalable Landmark-based version

Can be found here: https://github.com/KrainskiL/CGE.jl

The version here is for undirected, unweighted graphs. Moreover, the underlying model
assumes no loops (self-edges). The landmark-based version allows for weights and loops,
so numerical results can be slightly different.

# Comparing Graph Embeddings

We propose a framework to compare the quality of graph embeddings obtained with
different algorithms and/or different choices of parameters. To that effect, we
use a new divergence score based on a generalization of the well-know Chung-Lu
random graph model.

This repository contains the code and some examples for computing the divergence scores
for two versions of our framework (1) the original version, which is suitable for graphs
with thousands of vertices, and (2) a scalable, landmark-based version, which should be used
on graphs with millions of vertices.

## Details of the original framework can be found in: 

B. Kaminski, P. Pralat and F. Théberge, An unsupervised framework for comparing graph embeddings,
Journal of Complex Networks, cnz043, https://doi.org/10.1093/comnet/cnz043, Published: 28 November 2019.

Full paper: https://academic.oup.com/comnet/advance-article/doi/10.1093/comnet/cnz043/5645186?guestAccessKey=6c6efa8a-f322-4c00-aa26-7f067fec7eaf

Pre-print, arXiv:1906.04562 (2019).  https://arxiv.org/abs/1906.04562

## Details of the scalable, landmark-based version can be found in:

https://doi.org/10.1007/978-3-030-48478-1_4

# Compiling and Running the Original Framework 

GED (Graph Embedding Divergence)

This code computes the Jenssen-Shannon divergence between two edge distributions:  
(1) first one is based on the supplied graph clustering  
(2) second one is based on the supplied embedding and a Geometric Chung-Lu model  
When comparing embeddings, lower divergence is better.

To compile:

```
gcc GED.c -o GED -lm -O3
```

Format:

```
./GED -g edgelist_file -c clusters_file -e embedding_file [-a max_alpha -s epsilon_step -d delta -v]

## required flags:
-g: the edgelist (1 per line, whitespace separated, no weights)
-c: the communities (in vertex order, 1 per line)
-e: the embedding (node2vec format)
## optional flags:
-a: maximum value of alpha to consider (default 10.0)
-s: epsilon step (default 0.01)
-d: delta, stopping criterion (default 0.001)
-S: split and average the divergence measures (between and within communities respectively)
-v: verbose 
```

# Running with supplied data

The embeddings can be visualized in the supplied notebook.  

The embeddings were obtained with the node2vec, Verse and LINE algorithms (references below)  
We do not explicitely identify which embedding come from which algorithm, as this is not 
intended as a comparison study of those algorithms, just an illustration of the framework.

The clusterings were obtained with the ECG algorithm (reference below).  
The code can be found here: https://github.com/ftheberge/graph-partition-and-measures

```
./GED -g Data/karate.edgelist -c Data/karate.ecg -e Data/karate.embedding.1
Divergence: 3.812441e-03

./GED -g Data/karate.edgelist -c Data/karate.ecg -e Data/karate.embedding.2
Divergence: 6.264916e-03

./GED -g Data/karate.edgelist -c Data/karate.ecg -e Data/karate.embedding.3
Divergence: 6.616485e-02

./GED -g Data/lfr15.edgelist -c Data/lfr15.ecg -e Data/lfr15.embedding.1
Divergence: 2.288896e-04

./GED -g Data/lfr15.edgelist -c Data/lfr15.ecg -e Data/lfr15.embedding.2
Divergence: 4.494973e-02

./GED -g Data/lfr15.edgelist -c Data/lfr15.ecg -e Data/lfr15.embedding.3
Divergence: 1.196148e-01

./GED -g Data/football.edgelist -c Data/football.ecg -e Data/football.embedding.1
Divergence: 2.689874e-03

./GED -g Data/football.edgelist -c Data/football.ecg -e Data/football.embedding.2
Divergence: 2.464403e-02

./GED -g Data/football.edgelist -c Data/football.ecg -e Data/football.embedding.3
Divergence: 3.643811e-02
```

# File Formats

For a graph with n nodes, the nodes can be represented with numbers 1 to n or 0 to n-1.  

Three input files are required to run GED:  
1. the undirected, unweighted graph, represented by a sequence of edges, 1 per line  
2. a file with the node's cluster number, 1 per line, in numerical order of the nodes  
3. the node embedding in the node2vec format (see below)

## Example of graph (edgelist) file

Nodes can be 0-based or 1-based  
One edge per line with whitespace between nodes, no weights (so only 2 columns)

```
1 32
1 22
1 20
1 18
1 14
1 13
1 12
1 11
1 9
1 8
...
```

## Example of clustering file

Clusters can be 0-based or 1-based  
Clusters: one value per line in the numerical order of the nodes  

```
1
1
1
1
0
0
0
1
3
1
...
```
## Example of embedding file

Same format as with node2vec embedding:  
line 1:   #nodes dimension(d)   
lines 2+: node d-long_embedding_vector  

Nodes are 0-based or 1-based in any order  

```
34 8
21 0.960689 -2.28209 3.65194 0.272646 -3.01281 1.0245 -0.329389 -2.95956
33 0.702187 -2.14331 4.25541 0.372346 -3.16427 1.41296 -0.390471 -4.49782
3 0.854487 -2.30527 4.10575 0.370613 -3.04878 1.46481 -0.120326 -4.02328
29 0.673825 -2.19518 4.00447 0.650003 -2.74663 0.757385 -0.505723 -3.2947
32 0.750248 -2.26306 4.04495 0.143616 -3.02735 1.49937 -0.400896 -4.04177
25 0.831608 -2.191 4.04712 0.786012 -2.85804 1.11308 -0.391722 -3.4645
28 1.14632 -2.20708 4.11004 0.338067 -2.86409 1.01202 -0.485711 -3.50161
...
```

# Other References

Karate dataset:
Zachary, W. W. An information flow model for conflict and fission in small groups. Journal of Anthropological Research 33, 452–473  (1977)

ABCD Generator:
B. Kaminski, P. Pralat and F. Théberge, Artificial Benchmark for Community Detection (ABCD): Fast Random Graph Model with Community Structure, pre-print, arXiv 2002:00843 (2020). https://arxiv.org/abs/2002.00843

LFR Generator:
Lancichinetti, A., Fortunato, S. & Radicchi, F. Benchmark graphs for testing community detection algorithms. Physical Review E 78, 046110 (2008).

ECG (ensemble clustering for graphs):
V. Poulin and F. Théberge, Ensemble Clustering for Graphs: Comparison and Applications, Applied Network Science vol. 4, no. 51 (2019).

Original Chung-Lu model:
F. Chung and L.Lu, Complex Graphs and Networks, American Mathematical Society, 2006.

node2vec: A. Grover, J. Leskovec. node2vec: Scalable Feature Learning for Networks. KDD 2016: 855--864.

Verse: A. Tsitsulin, D. Mottin, P. Karras, and E. Müller. 2018. VERSE: Versatile Graph Embeddings from Similarity Measures. In Proceedings of the 2018 World Wide Web Conference (WWW'18). International World Wide Web Conferences Steering Committee, Republic and Canton of Geneva, Switzerland, 539-548.

LINE: J.  Tang,  M.  Qu,  M.  Wang,  M.  Zhang,  J.  Yan,  Q.  Mei,  Line:  Large-scale  information  network  embedding,  in:   Proceedings  24th  International Conference on World Wide Web, 2015, pp. 1067–1077.
