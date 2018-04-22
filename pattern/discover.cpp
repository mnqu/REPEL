#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <set>
#include <map>
#include <Eigen/Dense>

#define MAX_STRING 1000

const int hash_size = 30000000;  // Maximum 30 * 0.7 = 21M words in the vocabulary

typedef float real;                    // Precision of float numbers

typedef Eigen::Matrix< real, Eigen::Dynamic,
Eigen::Dynamic, Eigen::RowMajor | Eigen::AutoAlign >
BLPMatrix;

typedef Eigen::Matrix< real, 1, Eigen::Dynamic,
Eigen::RowMajor | Eigen::AutoAlign >
BLPVector;

const double MIN = -1000000;
double ratio = 0.1;

struct vocab_word
{
    char word[MAX_STRING];
};

struct triple
{
    int h, r, t;
    friend bool operator < (triple t1, triple t2)
    {
        if (t1.h == t2.h)
        {
            if (t1.r == t2.r) return t1.t < t2.t;
            return t1.r < t2.r;
        }
        return t1.h < t2.h;
    }
};

struct int_pair
{
    int u, v;
    friend bool operator < (int_pair p1, int_pair p2)
    {
        if (p1.u == p2.u) return p1.v < p2.v;
        return p1.u < p2.u;
    }
};

struct pair
{
    int id;
    real vl;
    friend bool operator < (pair p1, pair p2)
    {
        return p1.vl > p2.vl;
    }
};

struct kmax_list
{
    pair *list;
    int k_max, list_size;
    
    void init(int k)
    {
        k_max = k;
        list = (pair *)malloc((k_max + 1) * sizeof(pair));
        list_size = 0;
        for (int k = 0; k != k_max + 1; k++)
        {
            list[k].id = -1;
            list[k].vl = -1;
        }
    }
    
    void clear()
    {
        list_size = 0;
        for (int k = 0; k != k_max + 1; k++)
        {
            list[k].id = -1;
            list[k].vl = -1;
        }
    }
    
    void add(pair pr)
    {
        list[list_size].id = pr.id;
        list[list_size].vl = pr.vl;
        
        for (int k = list_size - 1; k >= 0; k--)
        {
            if (list[k].vl < list[k + 1].vl)
            {
                int tmp_id = list[k].id;
                real tmp_vl = list[k].vl;
                list[k].id = list[k + 1].id;
                list[k].vl = list[k + 1].vl;
                list[k + 1].id = tmp_id;
                list[k + 1].vl = tmp_vl;
            }
            else
            break;
        }
        
        if (list_size < k_max) list_size++;
    }
};

struct relation2data
{
    std::vector<triple> data;
    BLPMatrix heads, tails, dirs;
    int data_size, vector_size;
    
    void set(std::vector<triple> triples, BLPMatrix headvecs, BLPMatrix tailvecs, BLPMatrix directions)
    {
        data = triples;
        heads = headvecs;
        tails = tailvecs;
        dirs = directions;
        
        vector_size = (int)(dirs.cols());
        data_size = (int)(data.size());
    }
};

struct pid2score
{
    int pid;
    double score_d, score_s;
    friend bool operator < (pid2score p1, pid2score p2)
    {
        return p1.score_s + p1.score_d * ratio > p2.score_s + p2.score_d * ratio;
    }
};

char seed_file[MAX_STRING], fact_file[MAX_STRING], link_file[MAX_STRING], entity_file[MAX_STRING];
char out_pattern_file[MAX_STRING], out_fact_file[MAX_STRING];
struct vocab_word *entity, *relation;
int *entity_hash, *relation_hash;
int entity_size = 0, relation_size = 0, relation_max_size = 1000, seed_size = 0, valid_seed_size = 0, fact_size = 0, processed_fact_size = 0, infer_relation_id = -1;
long long link_size = 0;
int vector_size = 0, k_nns = 0, num_threads = 10, top_k = -1;
real lambda = 1, thresh_d = MIN, thresh_s = MIN, thresh_f = MIN;

BLPMatrix vec;
std::vector<triple> data_seed, data_fact;
relation2data *rlt2data;
std::vector<pair> *results;
std::map< int, std::set<int> > pid2fid;
std::map< int, std::set<int> > rlt2seedfid;
std::map< int, std::map<int, double> > rlt2fid2score;
std::map< int, double > fid2inferscore;
std::map< int_pair, int > fact2fid;
std::map< int, std::vector<pid2score> > rlt2pidscore;

long long hash(int h, int t)
{
    long long vl = h;
    vl = (vl << 32) + t;
    return vl;
}

// Returns hash value of a word
int GetWordHash(char *word) {
    unsigned long long a, hash = 0;
    for (a = 0; a < strlen(word); a++) hash = hash * 257 + word[a];
    hash = hash % hash_size;
    return hash;
}

// Returns position of a word in the vocabulary; if the word is not found, returns -1
int SearchEntity(char *word) {
    unsigned int hash = GetWordHash(word);
    while (1) {
        if (entity_hash[hash] == -1) return -1;
        if (!strcmp(word, entity[entity_hash[hash]].word)) return entity_hash[hash];
        hash = (hash + 1) % hash_size;
    }
    return -1;
}

// Adds a word to the vocabulary
int AddWordToEntity(char *word, int id) {
    unsigned int hash;
    strcpy(entity[id].word, word);
    hash = GetWordHash(word);
    while (entity_hash[hash] != -1) hash = (hash + 1) % hash_size;
    entity_hash[hash] = id;
    return id;
}

// Returns position of a word in the vocabulary; if the word is not found, returns -1
int SearchRelation(char *word) {
    unsigned int hash = GetWordHash(word);
    while (1) {
        if (relation_hash[hash] == -1) return -1;
        if (!strcmp(word, relation[relation_hash[hash]].word)) return relation_hash[hash];
        hash = (hash + 1) % hash_size;
    }
    return -1;
}

// Adds a word to the vocabulary
int AddWordToRelation(char *word) {
    unsigned int hash, length = strlen(word) + 1;
    strcpy(relation[relation_size].word, word);
    relation_size++;
    // Reallocate memory if needed
    if (relation_size + 2 >= relation_max_size) {
        relation_max_size += 1000;
        relation = (struct vocab_word *)realloc(relation, relation_max_size * sizeof(struct vocab_word));
    }
    hash = GetWordHash(word);
    while (relation_hash[hash] != -1) hash = (hash + 1) % hash_size;
    relation_hash[hash] = relation_size - 1;
    return relation_size - 1;
}

void ReadVector()
{
    FILE *fie;
    char ch, word[MAX_STRING];
    real f;
    
    fie = fopen(entity_file, "rb");
    if (fie == NULL) {
        printf("Vector file not found\n");
        exit(1);
    }
    
    fscanf(fie, "%d %d", &entity_size, &vector_size);
    
    entity = (struct vocab_word *)malloc(entity_size * sizeof(struct vocab_word));
    vec.resize(entity_size, vector_size);
    
    for (int k = 0; k != entity_size; k++)
    {
        fscanf(fie, "%s", word);
        ch = fgetc(fie);
        AddWordToEntity(word, k);
        for (int c = 0; c != vector_size; c++)
        {
            fread(&f, sizeof(real), 1, fie);
            //fscanf(fie, "%f", &f);
            vec(k, c) = f;
        }
        vec.row(k) /= vec.row(k).norm();
    }
    
    fclose(fie);
    
    printf("Entity size: %d\n", entity_size);
    printf("Vector size: %d\n", vector_size);
}

void ReadTriple()
{
    FILE *fi;
    char sh[MAX_STRING], st[MAX_STRING], sr[MAX_STRING];
    int h, t, r, id;
    triple trip;
    int_pair ipair;
    
    if (seed_file[0] != 0)
    {
        fi = fopen(seed_file, "rb");
        while (1)
        {
            if (fscanf(fi, "%s %s %s", sh, st, sr) != 3) break;
            
            h = SearchEntity(sh);
            t = SearchEntity(st);
            
            if (h == -1 || t == -1) continue;
            
            r = SearchRelation(sr);
            if (r == -1) r = AddWordToRelation(sr);
            
            seed_size++;
            
            trip.h = h; trip.r = r; trip.t = t;
            data_seed.push_back(trip);
        }
        fclose(fi);
    }
    
    fi = fopen(fact_file, "rb");
    while (1)
    {
        if (fscanf(fi, "%d %s %s", &id, sh, st) != 3) break;
        
        h = SearchEntity(sh);
        t = SearchEntity(st);
        
        if (h == -1 || t == -1) continue;
        fact_size++;
        
        trip.h = h; trip.t = t; trip.r = id;
        data_fact.push_back(trip);
        
        ipair.u = h; ipair.v = t;
        fact2fid[ipair] = id;
    }
    fclose(fi);
    
    printf("Relation size: %d\n", relation_size);
    printf("Seed size: %d\n", seed_size);
    printf("Fact size: %d\n", fact_size);
}

void Process()
{
    rlt2data = new relation2data [relation_size];
    std::vector<triple> triples;
    BLPMatrix directions, headvecs, tailvecs;
    for (int rr = 0; rr != relation_size; rr++)
    {
        triples.clear();
        for (int k = 0; k != seed_size; k++)
        {
            int r = data_seed[k].r;
            if (r != rr) continue;
            triples.push_back(data_seed[k]);
        }
        
        int size = (int)(triples.size());
        
        directions.resize(size, vector_size);
        headvecs.resize(size, vector_size);
        tailvecs.resize(size, vector_size);
        
        for (int k = 0; k != size; k++)
        {
            int h = triples[k].h;
            int t = triples[k].t;
            directions.row(k) = vec.row(t) - vec.row(h);
            headvecs.row(k) = vec.row(h);
            tailvecs.row(k) = vec.row(t);
        }
        
        rlt2data[rr].set(triples, headvecs, tailvecs, directions);
    }
}

void *Infer(void *id)
{
    long long tid = (long long)id;
    int bg = (int)(fact_size / num_threads * tid);
    int ed = (int)(fact_size / num_threads * (tid + 1));
    if (tid == num_threads - 1) ed = fact_size;
    
    int h, t, fid;
    int T = 0;
    pair pr;
    kmax_list nblist;
    nblist.init(k_nns);
    BLPVector dir, headvec, tailvec;
    dir.resize(vector_size);
    headvec.resize(vector_size);
    tailvec.resize(vector_size);
    double sc;
    double sum = 0;
    
    for (int data_id = bg; data_id != ed; data_id++)
    {
        T++;
        if (T % 10 == 0)
        {
            processed_fact_size += 10;
            printf("%cRelation: %d/%d Progress: %.2f%%", 13, infer_relation_id, relation_size, 100.0 * processed_fact_size / fact_size);
            fflush(stdout);
        }
        
        h = data_fact[data_id].h; t = data_fact[data_id].t;
        fid = data_fact[data_id].r;
        sc = 10;
        
        // use h + r to predict t
        nblist.clear();
        for (int k = 0; k != rlt2data[infer_relation_id].data_size; k++)
        {
            int hh = rlt2data[infer_relation_id].data[k].h;
            real f = vec.row(h) * vec.row(hh).transpose();
            pr.id = k; pr.vl = f;
            nblist.add(pr);
        }
        
        dir.setZero();
        headvec.setZero();
        tailvec.setZero();
        sum = 0;
        for (int k = 0; k != k_nns; k++)
        {
            int id = nblist.list[k].id;
            if (id == -1) continue;
            dir += rlt2data[infer_relation_id].dirs.row(id);
            headvec += rlt2data[infer_relation_id].heads.row(id);
            tailvec += rlt2data[infer_relation_id].tails.row(id);
            sum += 1;
        }
        if (sum != 0)
        {
            dir /= sum;
            headvec /= sum;
            tailvec /= sum;
        }
        
        sc -= (vec.row(h) + dir - vec.row(t)).norm();
        sc -= (vec.row(h) - headvec).norm() * lambda;
        sc -= (vec.row(t) - tailvec).norm() * lambda;
        
        // use t - r to predict h
        nblist.clear();
        for (int k = 0; k != rlt2data[infer_relation_id].data_size; k++)
        {
            int tt = rlt2data[infer_relation_id].data[k].t;
            real f = vec.row(t) * vec.row(tt).transpose();
            pr.id = k; pr.vl = f;
            nblist.add(pr);
        }
        
        dir.setZero();
        headvec.setZero();
        tailvec.setZero();
        sum = 0;
        for (int k = 0; k != k_nns; k++)
        {
            int id = nblist.list[k].id;
            if (id == -1) continue;
            dir += rlt2data[infer_relation_id].dirs.row(id);
            headvec += rlt2data[infer_relation_id].heads.row(id);
            tailvec += rlt2data[infer_relation_id].tails.row(id);
            sum += 1;
        }
        if (sum != 0)
        {
            dir /= sum;
            headvec /= sum;
            tailvec /= sum;
        }
        
        sc -= (vec.row(h) + dir - vec.row(t)).norm();
        sc -= (vec.row(h) - headvec).norm() * lambda;
        sc -= (vec.row(t) - tailvec).norm() * lambda;
        
        //sc /= 2;
        
        pr.id = fid;
        pr.vl = sc;
        results[tid].push_back(pr);
    }
    pthread_exit(NULL);
}

void ReadLink()
{
    int pid, fid;
    FILE *fi = fopen(link_file, "rb");
    while (fscanf(fi, "%d %d", &pid, &fid) == 2)
    {
        pid2fid[pid].insert(fid);
        link_size += 1;
    }
    fclose(fi);
    printf("Link size: %lld\n", link_size);
}

void ConsturctSeedSet()
{
    int h, t, r, fid;
    int_pair ipair;
    std::map<int_pair, int>::iterator iter;
    for (int k = 0; k != seed_size; k++)
    {
        h = data_seed[k].h;
        t = data_seed[k].t;
        r = data_seed[k].r;
        
        ipair.u = h; ipair.v = t;
        iter = fact2fid.find(ipair);
        if (iter == fact2fid.end()) continue;
        
        fid = iter->second;
        rlt2seedfid[r].insert(fid);
        valid_seed_size += 1;
    }
    printf("Valid seed size: %d\n", valid_seed_size);
}

void CalculatePatternScore()
{
    int pid, fid;
    pid2score pidscore;
    std::vector<pid2score> ranklist;
    std::map<int, std::set<int> >::iterator iterIntSet;
    std::set<int>::iterator iterInt;
    
    FILE *fo = fopen(out_pattern_file, "wb");
    for (int rlt = 0; rlt != relation_size; rlt++)
    {
        rlt2pidscore[rlt].clear();
        ranklist.clear();
        for (iterIntSet = pid2fid.begin(); iterIntSet != pid2fid.end(); iterIntSet++)
        {
            double sm_s = 0, sm_d = 0;
            int cn_s = 0, cn_d = 0;
            double score_s = 0, score_d = 0;
            
            pid = iterIntSet->first;
            for (iterInt = (iterIntSet->second).begin(); iterInt != (iterIntSet->second).end(); iterInt++)
            {
                fid = *iterInt;
                
                if (rlt2seedfid[rlt].count(fid)) sm_s += 1;
                cn_s += 1;
                
                sm_d += rlt2fid2score[rlt][fid];
                cn_d += 1;
            }
            score_s = MIN; score_d = MIN;
            if (sm_s != 0 && cn_s != 0) score_s = sm_s / cn_s;
            if (cn_d != 0) score_d = sm_d / cn_d;
            
            if (score_s == MIN) continue;
            pidscore.pid = pid;
            pidscore.score_d = score_d;
            pidscore.score_s = score_s;
            ranklist.push_back(pidscore);
        }
        std::sort(ranklist.begin(), ranklist.end());
        for (int k = 0; k != (int)(ranklist.size()); k++)
        {
            if (k == top_k) break;
            
            pid = ranklist[k].pid;
            double score_d = ranklist[k].score_d;
            double score_s = ranklist[k].score_s;
            
            if (score_d < thresh_d) continue;
            if (score_s < thresh_s) continue;
            
            rlt2pidscore[rlt].push_back(ranklist[k]);
            fprintf(fo, "%s\t%d\t%lf\t%lf\n", relation[rlt].word, pid, score_s, score_d);
        }
    }
    fclose(fo);
}

void CalculateFactScore()
{
    int pid, fid, length;
    double score;
    pair pr;
    std::vector<pair> ranklist;
    std::map<int, std::set<int> >::iterator iterIntSet;
    std::set<int>::iterator iterInt;
    std::map<int, double>::iterator iterIntDouble;
    
    FILE *fo = fopen(out_fact_file, "wb");
    for (int rlt = 0; rlt != relation_size; rlt++)
    {
        fid2inferscore.clear();
        length = rlt2pidscore[rlt].size();
        for (int k = 0; k != length; k++)
        {
            pid = rlt2pidscore[rlt][k].pid;
            score = rlt2pidscore[rlt][k].score_s;
            
            iterIntSet = pid2fid.find(pid);
            if (iterIntSet == pid2fid.end()) continue;
            
            for (iterInt = (iterIntSet->second).begin(); iterInt != (iterIntSet->second).end(); iterInt++)
            {
                fid = *iterInt;
                fid2inferscore[fid] += score;
            }
        }
        ranklist.clear();
        for (iterIntDouble = fid2inferscore.begin(); iterIntDouble != fid2inferscore.end(); iterIntDouble++)
        {
            pr.id = iterIntDouble->first;
            pr.vl = iterIntDouble->second;
            ranklist.push_back(pr);
        }
        std::sort(ranklist.begin(), ranklist.end());
        length = ranklist.size();
        for (int k = 0; k != length; k++)
        {
            int id = ranklist[k].id;
            double vl = ranklist[k].vl;
            if (vl < thresh_f) continue;
            fprintf(fo, "%s\t%d\t%lf\n", relation[rlt].word, id, vl);
        }
    }
    fclose(fo);
}

void TrainModel()
{
    entity_hash = (int *)calloc(hash_size, sizeof(int));
    for (long long a = 0; a < hash_size; a++) entity_hash[a] = -1;
    relation = (struct vocab_word *)calloc(relation_max_size, sizeof(struct vocab_word));
    relation_hash = (int *)calloc(hash_size, sizeof(int));
    for (long long a = 0; a < hash_size; a++) relation_hash[a] = -1;
    
    results = new std::vector<pair> [num_threads];
    pthread_t *pt = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    
    ReadVector();
    ReadTriple();
    Process();
    for (infer_relation_id = 0; infer_relation_id != relation_size; infer_relation_id++)
    {
        for (int k = 0; k != num_threads; k++) results[k].clear();
        
        processed_fact_size = 0;
        for (long a = 0; a < num_threads; a++) pthread_create(&pt[a], NULL, Infer, (void *)a);
        for (long a = 0; a < num_threads; a++) pthread_join(pt[a], NULL);
        
        for (int k = 0; k != num_threads; k++)
        {
            int length = results[k].size();
            for (int i = 0; i != length; i++) rlt2fid2score[infer_relation_id][results[k][i].id] = results[k][i].vl;
        }
    }
    printf("\n");
    
    ReadLink();
    ConsturctSeedSet();
    CalculatePatternScore();
    CalculateFactScore();
}

int ArgPos(char *str, int argc, char **argv) {
    int a;
    for (a = 1; a < argc; a++) if (!strcmp(str, argv[a])) {
        if (a == argc - 1) {
            printf("Argument missing for %s\n", str);
            exit(1);
        }
        return a;
    }
    return -1;
}

int main(int argc, char **argv) {
    int i;
    if (argc == 1) {
        return 0;
    }
    if ((i = ArgPos((char *)"-seed", argc, argv)) > 0) strcpy(seed_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-fact", argc, argv)) > 0) strcpy(fact_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-link", argc, argv)) > 0) strcpy(link_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-entity", argc, argv)) > 0) strcpy(entity_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-output-pattern", argc, argv)) > 0) strcpy(out_pattern_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-output-fact", argc, argv)) > 0) strcpy(out_fact_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-knns", argc, argv)) > 0) k_nns = atof(argv[i + 1]);
    if ((i = ArgPos((char *)"-lambda", argc, argv)) > 0) lambda = atof(argv[i + 1]);
    if ((i = ArgPos((char *)"-top-k", argc, argv)) > 0) top_k = atoi(argv[i + 1]);
    if ((i = ArgPos((char *)"-thresh-d", argc, argv)) > 0) thresh_d = atof(argv[i + 1]);
    if ((i = ArgPos((char *)"-thresh-s", argc, argv)) > 0) thresh_s = atof(argv[i + 1]);
    if ((i = ArgPos((char *)"-thresh-f", argc, argv)) > 0) thresh_f = atof(argv[i + 1]);
    if ((i = ArgPos((char *)"-threads", argc, argv)) > 0) num_threads = atoi(argv[i + 1]);
    TrainModel();
    return 0;
}
