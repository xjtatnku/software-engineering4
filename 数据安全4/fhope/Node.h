#pragma once

#include <map>
#include <string>
#include <vector>

using namespace std;

const int M = 128;
const long long MAX_CODE = 1LL << 62;

class Node
{
public:
    int type;       // 1: LeafNode, 2: InternalNode
    int parent_index;
    Node *parent = nullptr;

    virtual ~Node() = default;
    virtual void rebalance() = 0;
    virtual long long insert(int pos, string cipher) = 0;
    virtual long long search(int pos) = 0;
};

class InternalNode : public Node
{
public:
    vector<int> child_num;
    vector<Node *> child;

    InternalNode();
    void rebalance() override;
    long long insert(int pos, string cipher) override;
    long long search(int pos) override;
    void insert_node(int index, Node *new_node);
};

class LeafNode : public Node
{
public:
    vector<string> cipher;
    vector<long long> encoding;
    LeafNode *left_bro = nullptr;
    LeafNode *right_bro = nullptr;
    long long lower = -1;
    long long upper = -1;

    LeafNode();
    long long Encode(int pos);
    void rebalance() override;
    long long insert(int pos, string cipher) override;
    long long search(int pos) override;
};

struct TreeStats
{
    long long total_count = 0;
    long long leaf_count = 0;
    long long internal_count = 0;
    long long height = 0;
    long long max_leaf_size = 0;
};

extern Node *root;
extern long long start_update;
extern long long end_update;
extern map<string, long long> update;

void root_initial();
long long get_update(string cipher);
TreeStats tree_stats();
