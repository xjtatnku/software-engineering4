#include "Node.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Row
{
    std::string cipher;
    long long encoding;
};

int main(int argc, char **argv)
{
    int count = 140;
    int every = 10;
    if (argc >= 2)
    {
        count = std::max(1, std::stoi(argv[1]));
    }
    if (argc >= 3)
    {
        every = std::max(1, std::stoi(argv[2]));
    }

    root_initial();
    std::vector<Row> rows;
    TreeStats previous = tree_stats();

    std::cout << "i pos returned rows changed update_range height leaves max_leaf event\n";

    for (int i = 1; i <= count; i++)
    {
        std::vector<long long> before;
        before.reserve(rows.size());
        for (const Row &row : rows)
        {
            before.push_back(row.encoding);
        }

        std::string cipher = "ct_" + std::to_string(i);
        int pos = 0; // Same-value left insertion keeps squeezing the same coding interval.
        start_update = -1;
        end_update = -1;
        update.clear();
        long long inserted = root->insert(pos, cipher);
        rows.insert(rows.begin() + pos, Row{cipher, inserted});

        if (inserted == 0)
        {
            for (Row &row : rows)
            {
                if ((row.encoding >= start_update && row.encoding < end_update) || row.encoding == 0)
                {
                    row.encoding = get_update(row.cipher);
                }
            }
        }

        int changed = 0;
        for (int j = 0; j < static_cast<int>(before.size()); j++)
        {
            if (before.at(j) != rows.at(j + 1).encoding)
            {
                changed++;
            }
        }

        TreeStats stats = tree_stats();
        bool recode = inserted == 0 || start_update >= 0 || changed > 0;
        bool split = stats.leaf_count != previous.leaf_count || stats.height != previous.height;

        if (i <= 5 || i == count || i % every == 0 || recode || split)
        {
            std::string event = "-";
            if (recode && split)
            {
                event = "recode,split";
            }
            else if (recode)
            {
                event = "recode";
            }
            else if (split)
            {
                event = "split";
            }

            std::cout << i << ' ' << pos << ' ' << inserted << ' ' << rows.size() << ' '
                      << changed << " [" << start_update << ',' << end_update << ") "
                      << stats.height << ' ' << stats.leaf_count << ' ' << stats.max_leaf_size
                      << ' ' << event << '\n';
        }

        previous = stats;
    }

    return 0;
}
