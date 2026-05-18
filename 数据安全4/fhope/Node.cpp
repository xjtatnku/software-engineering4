#include "Node.h"

#include <algorithm>
#include <cassert>
#include <cmath>

Node *root = nullptr;
long long start_update = -1;
long long end_update = -1;
map<string, long long> update;

namespace
{
void free_tree(Node *node)
{
    if (!node)
    {
        return;
    }
    if (node->type == 2)
    {
        InternalNode *internal = static_cast<InternalNode *>(node);
        for (Node *child : internal->child)
        {
            free_tree(child);
        }
    }
    delete node;
}

int subtree_size(Node *node)
{
    if (!node)
    {
        return 0;
    }
    if (node->type == 1)
    {
        return static_cast<int>(static_cast<LeafNode *>(node)->cipher.size());
    }
    int total = 0;
    InternalNode *internal = static_cast<InternalNode *>(node);
    for (int num : internal->child_num)
    {
        total += num;
    }
    return total;
}

void collect_stats(Node *node, long long depth, TreeStats &stats)
{
    if (!node)
    {
        return;
    }
    stats.height = max(stats.height, depth);
    if (node->type == 1)
    {
        LeafNode *leaf = static_cast<LeafNode *>(node);
        stats.leaf_count++;
        stats.total_count += static_cast<long long>(leaf->cipher.size());
        stats.max_leaf_size = max(stats.max_leaf_size, static_cast<long long>(leaf->cipher.size()));
        return;
    }

    stats.internal_count++;
    InternalNode *internal = static_cast<InternalNode *>(node);
    for (Node *child : internal->child)
    {
        collect_stats(child, depth + 1, stats);
    }
}
} // namespace

InternalNode::InternalNode()
{
    this->type = 2;
    this->parent_index = -1;
    this->parent = nullptr;
}

void InternalNode::insert_node(int index, Node *new_node)
{
    this->child.insert(this->child.begin() + index, new_node);
    this->child_num.insert(this->child_num.begin() + index, subtree_size(new_node));
    new_node->parent = this;

    for (int i = 0; i < static_cast<int>(this->child.size()); i++)
    {
        this->child.at(i)->parent_index = i;
        this->child_num.at(i) = subtree_size(this->child.at(i));
    }

    if (this->child.size() >= M)
    {
        this->rebalance();
    }
}

void InternalNode::rebalance()
{
    InternalNode *new_node = new InternalNode();
    int middle = static_cast<int>(floor(this->child.size() * 0.5));

    while (middle > 0)
    {
        new_node->child.insert(new_node->child.begin(), this->child.back());
        new_node->child_num.insert(new_node->child_num.begin(), this->child_num.back());
        this->child.pop_back();
        this->child_num.pop_back();
        middle--;
    }

    for (int i = 0; i < static_cast<int>(new_node->child.size()); i++)
    {
        new_node->child.at(i)->parent_index = i;
        new_node->child.at(i)->parent = new_node;
    }

    if (!this->parent)
    {
        InternalNode *new_root = new InternalNode();
        new_root->insert_node(0, this);
        new_root->insert_node(1, new_node);
        root = new_root;
    }
    else
    {
        static_cast<InternalNode *>(this->parent)->child_num.at(this->parent_index) = subtree_size(this);
        static_cast<InternalNode *>(this->parent)->insert_node(this->parent_index + 1, new_node);
    }
}

long long InternalNode::insert(int pos, string cipher)
{
    if (this->child.empty())
    {
        return 0;
    }

    for (int i = 0; i < static_cast<int>(this->child.size()); i++)
    {
        if (pos > this->child_num.at(i))
        {
            pos -= this->child_num.at(i);
        }
        else
        {
            this->child_num.at(i)++;
            return this->child.at(i)->insert(pos, cipher);
        }
    }

    this->child_num.back()++;
    return this->child.back()->insert(pos, cipher);
}

long long InternalNode::search(int pos)
{
    if (this->child.empty())
    {
        return 0;
    }

    for (int i = 0; i < static_cast<int>(this->child.size()); i++)
    {
        if (pos < this->child_num.at(i))
        {
            return this->child.at(i)->search(pos);
        }
        pos -= this->child_num.at(i);
    }

    return this->child.back()->search(this->child_num.back());
}

LeafNode::LeafNode()
{
    this->type = 1;
    this->parent_index = -1;
    this->parent = nullptr;
}

void Recode(vector<LeafNode *> node_list)
{
    long long left_bound = node_list.at(0)->lower;
    long long right_bound = node_list.back()->upper;
    int total_cipher_num = 0;

    for (LeafNode *node : node_list)
    {
        total_cipher_num += static_cast<int>(node->cipher.size());
    }

    if (total_cipher_num <= 0)
    {
        return;
    }

    if ((right_bound - left_bound) > total_cipher_num)
    {
        start_update = left_bound;
        end_update = right_bound;
        long long frag = static_cast<long long>(floor((right_bound - left_bound) / total_cipher_num));
        assert(frag >= 1);

        long long cd = left_bound;
        for (LeafNode *node : node_list)
        {
            node->lower = cd;
            for (int j = 0; j < static_cast<int>(node->encoding.size()); j++)
            {
                node->encoding.at(j) = cd;
                update[node->cipher.at(j)] = cd;
                cd += frag;
            }
            node->upper = cd;
        }
        node_list.back()->upper = right_bound;
    }
    else
    {
        if (node_list.at(0)->left_bro)
        {
            node_list.insert(node_list.begin(), node_list.at(0)->left_bro);
        }

        if (node_list.back()->right_bro)
        {
            node_list.push_back(node_list.back()->right_bro);
        }
        else
        {
            if (node_list.back()->upper > MAX_CODE / 2)
            {
                node_list.back()->upper = MAX_CODE;
            }
            else
            {
                node_list.back()->upper *= 2;
            }
        }
        Recode(node_list);
    }
}

long long LeafNode::Encode(int pos)
{
    long long left = this->lower;
    long long right = this->upper;

    if (pos > 0)
    {
        left = this->encoding.at(pos - 1);
    }
    if (pos + 1 < static_cast<int>(this->encoding.size()))
    {
        right = this->encoding.at(pos + 1);
    }

    if (floor(right - left) < 2)
    {
        vector<LeafNode *> node_list;
        node_list.push_back(this);
        Recode(node_list);
        return 0;
    }

    long long frag = (right - left) / 2;
    this->encoding.at(pos) = right - frag;
    return this->encoding.at(pos);
}

void LeafNode::rebalance()
{
    LeafNode *new_node = new LeafNode();
    int middle = static_cast<int>(floor(this->cipher.size() * 0.5));

    while (middle > 0)
    {
        new_node->cipher.insert(new_node->cipher.begin(), this->cipher.back());
        new_node->encoding.insert(new_node->encoding.begin(), this->encoding.back());
        this->encoding.pop_back();
        this->cipher.pop_back();
        middle--;
    }

    new_node->lower = new_node->encoding.at(0);
    new_node->upper = this->upper;
    this->upper = new_node->encoding.at(0);

    if (this->right_bro)
    {
        this->right_bro->left_bro = new_node;
    }
    new_node->right_bro = this->right_bro;
    this->right_bro = new_node;
    new_node->left_bro = this;

    if (!this->parent)
    {
        InternalNode *new_root = new InternalNode();
        new_root->insert_node(0, this);
        new_root->insert_node(1, new_node);
        root = new_root;
    }
    else
    {
        static_cast<InternalNode *>(this->parent)->child_num.at(this->parent_index) = static_cast<int>(this->cipher.size());
        static_cast<InternalNode *>(this->parent)->insert_node(this->parent_index + 1, new_node);
    }
}

long long LeafNode::insert(int pos, string cipher)
{
    pos = max(0, min(pos, static_cast<int>(this->cipher.size())));
    this->cipher.insert(this->cipher.begin() + pos, cipher);
    this->encoding.insert(this->encoding.begin() + pos, -1);
    long long cd = this->Encode(pos);

    if (this->cipher.size() >= M)
    {
        this->rebalance();
    }
    return cd;
}

long long LeafNode::search(int pos)
{
    if (this->encoding.empty())
    {
        return this->upper;
    }
    if (pos < 0)
    {
        return this->lower;
    }
    if (pos >= static_cast<int>(this->encoding.size()))
    {
        return this->upper;
    }
    return this->encoding.at(pos);
}

void root_initial()
{
    free_tree(root);
    root = new LeafNode();
    static_cast<LeafNode *>(root)->lower = 0;
    static_cast<LeafNode *>(root)->upper = MAX_CODE;
    update.clear();
    start_update = -1;
    end_update = -1;
}

long long get_update(string cipher)
{
    if (update.count(cipher) > 0)
    {
        return update[cipher];
    }
    return 0;
}

TreeStats tree_stats()
{
    TreeStats stats;
    collect_stats(root, root ? 1 : 0, stats);
    return stats;
}
