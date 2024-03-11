#include <BMCP.hpp>
#include <cstdlib>
#include <cstdio>
#include <cassert>

void BMCP::BMCPSolver::Add_Item(const int item)
{
    solution_size++;
    solution[item] = 1;
    solution_weight_sum += g->weight[item];

    for (int elem_nei: g->item_neighbor[item])
    {
        solution_elements[elem_nei] += 1;
        if (solution_elements[elem_nei] == 1)
        {
            solution_profit_sum += g->profit[elem_nei];
            for (int item_nei: g->element_neighbor[elem_nei])
            {
                if (solution[item_nei]) [[unlikely]] continue;
                solution_contribution[item_nei] -= g->profit[elem_nei];
            }
        }
        else if (solution_elements[elem_nei] == 2)
        {
            for (int item_nei: g->element_neighbor[elem_nei])
            {
                if (!solution[item_nei]) [[likely]] continue;
                if (item_nei == item) [[unlikely]] continue;
                solution_contribution[item_nei] -= g->profit[elem_nei];
                break;
            }
        }
    }

    //check_solution();
}

void BMCP::BMCPSolver::Remove_Item(const int item)
{
    solution_size--;
    solution[item] = 0;
    solution_weight_sum -= g->weight[item];

    for (int elem_nei: g->item_neighbor[item])
    {
        solution_elements[elem_nei]--;
        if (solution_elements[elem_nei] == 0)
        {
            solution_profit_sum -= g->profit[elem_nei];
            for (int item_nei: g->element_neighbor[elem_nei])
            {
                if (item_nei == item) [[unlikely]] continue;
                solution_contribution[item_nei] += g->profit[elem_nei];
            }
        }
        else if (solution_elements[elem_nei] == 1)
        {
            for (int item_nei: g->element_neighbor[elem_nei])
            {
                if (!solution[item_nei])[[likely]] continue;
                solution_contribution[item_nei] += g->profit[elem_nei];
                break;
            }
        }
    }

    //check_solution();
}

void BMCP::BMCPSolver::Add_Item_With_Conf_Change(const int item, const int iter)
{
    solution_size++;
    solution[item] = 1;
    solution_weight_sum += g->weight[item];

    origin_conf_change_in_solution[item] = conf_change_in_solution[item] = solution_contribution[item];

    conf_change_timestamp[item] = iter;

    for (int elem_nei: g->item_neighbor[item])
    {
        solution_elements[elem_nei]++;
        if (solution_elements[elem_nei] == 1)
        {
            solution_profit_sum += g->profit[elem_nei];

            element_select_time[elem_nei] = iter;

            for (int item_nei: g->element_neighbor[elem_nei])
            {
                if (solution[item_nei]) [[unlikely]] continue;
                solution_contribution[item_nei] -= g->profit[elem_nei];
                conf_change_out_of_solution[item_nei] -= g->profit[elem_nei];
            }
        }
        else if (solution_elements[elem_nei] == 2)
        {
            for (int item_nei: g->element_neighbor[elem_nei])
            {
                if (!solution[item_nei]) [[likely]] continue;
                if (item_nei == item) [[unlikely]] continue;
                solution_contribution[item_nei] -= g->profit[elem_nei];
                conf_change_in_solution[item_nei] -= g->profit[elem_nei];
                break;
            }
        }
    }

    //check_solution();
}

void BMCP::BMCPSolver::Remove_Item_With_Conf_Change(const int item, const int iter)
{
    solution_size--;
    solution[item] = 0;
    solution_weight_sum -= g->weight[item];

    origin_conf_change_out_of_solution[item] = conf_change_out_of_solution[item] = solution_contribution[item];

    conf_change_timestamp[item] = iter;

    for (int elem_nei: g->item_neighbor[item])
    {
        solution_elements[elem_nei]--;
        if (solution_elements[elem_nei] == 0)
        {
            solution_profit_sum -= g->profit[elem_nei];

            element_satisfied_time[elem_nei] = iter - element_select_time[elem_nei];

            for (int item_nei: g->element_neighbor[elem_nei])
            {
                if (item_nei == item) [[unlikely]]continue;
                solution_contribution[item_nei] += g->profit[elem_nei];
                conf_change_out_of_solution[item_nei] -= g->profit[elem_nei];
            }
        }
        else if (solution_elements[elem_nei] == 1)
        {
            for (int item_nei: g->element_neighbor[elem_nei])
            {
                if (!solution[item_nei]) [[likely]] continue;
                solution_contribution[item_nei] += g->profit[elem_nei];
                conf_change_in_solution[item_nei] -= g->profit[elem_nei];
                break;
            }
        }
    }

    //check_solution();
}

void BMCP::BMCPSolver::Solution_To_Best_Solution()
{
    best_solution_profit_sum = solution_profit_sum;
    best_solution_weight_sum = solution_weight_sum;
    best_solution_size = solution_size;
    for (int i = 1; i <= g->m; i++)
    {
        best_solution[i] = solution[i];
        best_solution_contribution[i] = solution_contribution[i];
    }
    for (int i = 1; i <= g->n; i++)
    {
        best_solution_elements[i] = solution_elements[i];
    }
}

void BMCP::BMCPSolver::Solution_To_Star_Solution()
{
    star_solution_profit_sum = solution_profit_sum;
    star_solution_weight_sum = solution_weight_sum;
    star_solution_size = solution_size;
    for (int i = 1; i <= g->m; i++)
    {
        star_solution[i] = solution[i];
        star_solution_contribution[i] = solution_contribution[i];
    }
    for (int i = 1; i <= g->n; i++)
    {
        star_solution_elements[i] = solution_elements[i];
    }
}

void BMCP::BMCPSolver::Best_Solution_To_Solution()
{
    solution_profit_sum = best_solution_profit_sum;
    solution_weight_sum = best_solution_weight_sum;
    solution_size = best_solution_size;
    for (int i = 1; i <= g->m; i++)
    {
        solution[i] = best_solution[i];
        solution_contribution[i] = best_solution_contribution[i];
    }
    for (int i = 1; i <= g->n; i++)
    {
        solution_elements[i] = best_solution_elements[i];
    }
}

void BMCP::BMCPSolver::Greedy_Initialization()
{
    //init in_solution
    in_solution.clear();
    while (solution_weight_sum <= g->C && solution_size < g->m)
    {
        //select the item with the highest density under the premise that total weight <= C
        int ustar = -1;
        for (int i = 1; i <= g->m; i++)
        {
            if (solution[i]) [[unlikely]] continue;
            if (g->weight[i] + solution_weight_sum > g->C) continue;
            if (solution_contribution[i] == 0) [[unlikely]]continue;
            if (ustar == -1) [[unlikely]]
            {
                ustar = i;
                continue;
            }
            if (solution_contribution[i] * g->weight[ustar] > solution_contribution[ustar] * g->weight[i])
                ustar = i;
        }
        if (ustar != -1)
        {
            Add_Item(ustar);
            //printf("add %d\n",ustar);
            in_solution.insert(ustar);
            for (int i = 0; i < in_solution.size();)
            {
                int item = in_solution[i];
                if (solution_contribution[item] == 0)
                {
                    in_solution.erase(i);
                    Remove_Item(item);
                    //printf("remove %d\n",item);
                }
                else
                    i++;
            }
        }
        else
            break;
    }

    //init cc
    for (int i = 1; i <= g->m; i++)
    {
        conf_change_out_of_solution[i] = 0;
        origin_conf_change_out_of_solution[i] = 1;
        conf_change_in_solution[i] = 0;
        origin_conf_change_in_solution[i] = 1;
        conf_change_timestamp[i] = 0;
    }
    //init estimated value and select times
    for (int i = 1; i <= g->m; i++)
    {
        select_times[i] = solution[i];
        r_sum[i] = solution_contribution[i];
    }
}

int BMCP::BMCPSolver::Multiple_Selections(const int amount)
{
    if (random_list.size() <= amount) [[unlikely]]
        return random_list.size() - 1;
    for (int i = 0; i < amount; i++)
    {
        //std::uniform_int_distribution<int> dis(i, random_list.size() - 1);
        int random_num = rand() % (random_list.size() - i) + i;//dis(*linear_rand);
        std::swap(random_list[random_num], random_list[i]);
    }
    return amount - 1;
}

double BMCP::BMCPSolver::Upper_Confidence_Bound(const int item)
{
    return (double) r_sum[item] / select_times[item];
}

double BMCP::BMCPSolver::r(const int item)
{
    return (double) solution_contribution[item] / (solution_weight_sum + g->weight[item] - g->C);
}

void BMCP::BMCPSolver::CC_Search()
{
    Solution_To_Best_Solution();

    in_solution.clear();
    for (int i = 1; i <= g->m; i++)
    {
        if (solution[i])
            in_solution.insert(i);
    }

    for (int i = 1; i <= g->n; i++)
    {
        element_select_time[i] = 0;
    }

    int iter = 0;
    while (iter < Imax1)
    {
        if (solution_weight_sum > g->C)
        {
            //select an item in solution with the lowest density
            int ustar = -1;
            int ustar_idx = -1;
            for (int i = 0; i < in_solution.size(); i++)
            {
                int item = in_solution[i];
                if (conf_change_in_solution[item] > origin_conf_change_in_solution[item] * lambda &&
                    iter - conf_change_timestamp[item] < rand_deviation(timestamp_gap))
                    continue;
                if (ustar == -1) [[unlikely]]
                {
                    ustar = item;
                    ustar_idx = i;
                    continue;
                }
                if (solution_contribution[item] * g->weight[ustar] <
                    solution_contribution[ustar] * g->weight[item])
                {
                    ustar = item;
                    ustar_idx = i;
                }
            }
            if (ustar != -1) [[likely]]
            {
                in_solution.erase(ustar_idx);
                Remove_Item_With_Conf_Change(ustar, iter);
            }
        }
        //printf("%d %d %d\n", iter, solution_weight_sum, solution_profit_sum);
        //fflush(stdout);
        if (solution_weight_sum <= g->C && solution_profit_sum > best_solution_profit_sum)
        {
            Solution_To_Best_Solution();
        }

        if (solution_weight_sum <= g->C)
        {
            bool get_into_local_optimum = true;
            random_list.clear();
            for (int i = 1; i <= g->m; i++)
            {
                if (solution[i]) [[unlikely]]continue;
                if (solution_weight_sum + g->weight[i] > g->C) continue;
                if (solution_contribution[i] == 0) continue;
                if (conf_change_out_of_solution[i] >= origin_conf_change_out_of_solution[i] * lambda &&
                    iter - conf_change_timestamp[i] < rand_deviation(timestamp_gap))
                    continue;
                random_list.push_back(i);
                get_into_local_optimum = false;
            }
            if (!get_into_local_optimum)
            {
                int idx = Multiple_Selections(30);
                int ustar = -1;
                for (int i = 0; i <= idx; i++)
                {
                    int item = random_list[i];
                    if (ustar == -1) [[unlikely]]
                    {
                        ustar = item;
                        continue;
                    }
                    if (solution_contribution[item] * g->weight[ustar] >
                        solution_contribution[ustar] * g->weight[item])
                        ustar = item;
                }
                if (ustar != -1) [[likely]]
                {
                    in_solution.insert(ustar);
                    Add_Item_With_Conf_Change(ustar, iter);
                }
            }
            else
            {
                int ustar = -1;
                double ustar_ucb;
                double tmp_ucb;
                for (int i = 1; i <= g->m; i++)
                {
                    if (solution[i]) continue;
                    if (conf_change_out_of_solution[i] >= origin_conf_change_out_of_solution[i] * lambda &&
                        iter - conf_change_timestamp[i] < rand_deviation(timestamp_gap))
                        continue;
                    if (ustar == -1)
                    {
                        ustar = i;
                        ustar_ucb = Upper_Confidence_Bound(ustar);
                        continue;
                    }
                    if ((tmp_ucb = Upper_Confidence_Bound(i)) > ustar_ucb)
                    {
                        ustar = i;
                        ustar_ucb = tmp_ucb;
                    }
                }
                if (ustar != -1)
                {
                    in_solution.insert(ustar);
                    r_sum[ustar] += r(ustar);
                    select_times[ustar]++;
                    Add_Item_With_Conf_Change(ustar, iter);
                }
            }
        }
        //printf("%d %d %d\n", iter, solution_weight_sum, solution_profit_sum);
        //fflush(stdout);
        if (solution_weight_sum <= g->C && solution_profit_sum > best_solution_profit_sum)
        {
            Solution_To_Best_Solution();
        }
        iter++;
    }

    for (int i = 1; i <= g->n; i++)
    {
        if (solution_elements[i])
            element_satisfied_time[i] = Imax1 - element_select_time[i];
    }
}

void BMCP::BMCPSolver::Deep_Optimize()
{
    Solution_To_Best_Solution();

    //init in_solution
    in_solution.clear();
    for (int i = 1; i <= g->m; i++)
    {
        if (solution[i])
            in_solution.insert(i);
    }

    //init block_list
    int block_weight_sum = 0;
    for (int i = 1; i <= g->m; i++)
        block_list[i] = 0;

    random_list.clear();
    for (int i = 1; i <= g->n; i++)
    {
        random_list.push_back(i);
    }
    for (int i = 0; i < block_list_size; i++)
    {
        int ustar = -1;
        int ustar_idx;
        for (int j = i + 1; j < g->n; j++)
        {
            int elem = random_list[j];
            if (ustar == -1)
            {
                ustar = elem;
                ustar_idx = j;
                continue;
            }
            if (element_select_time[elem] < element_select_time[ustar])
            {
                ustar = elem;
                ustar_idx = j;
            }
        }
        if(ustar != -1)
        {
            std::swap(random_list[ustar_idx], random_list[i]);
        }
    }
    for (int i = 0; i < block_list_size; i++)
    {
        int elem = random_list[i];
        if (solution_elements[elem]) continue;

        int ustar = -1;
        for (int item_nei: g->element_neighbor[elem])
        {
            if (ustar == -1) [[unlikely]]
            {
                ustar = item_nei;
                continue;
            }
            if (solution_contribution[item_nei] * g->weight[ustar] >
                solution_contribution[ustar] * g->weight[item_nei])
                ustar = item_nei;
        }
        if (ustar != -1) [[likely]]
        {
            if (block_weight_sum + g->weight[ustar] > g->C) continue;
            Add_Item(ustar);
            in_solution.insert(ustar);
            block_list[ustar] = 1;
            block_weight_sum += g->weight[ustar];
        }
    }
    //init tabu
    for (int i = 1; i <= g->m; i++)
    {
        tabu_list[i] = 0;
    }
    int iter = 0;
    while (iter < Imax2)
    {
        if (solution_weight_sum < g->C)
        {
            int ustar = -1;
            for (int i = 1; i <= g->m; i++)
            {
                if (solution[i]) continue;
                if (solution_profit_sum + solution_contribution[i] > best_solution_profit_sum &&
                    solution_weight_sum + g->weight[i] <= g->C)
                {
                    ustar = i;
                    continue;
                }
                if (iter - tabu_list[i] < rand_deviation(tabu_length)) continue;
                if (ustar == -1)
                {
                    ustar = i;
                    continue;
                }
                if (solution_contribution[i] * g->weight[ustar] >
                    solution_contribution[ustar] * g->weight[i])
                {
                    ustar = i;
                }
            }
            if (ustar != -1)
            {
                Add_Item(ustar);
                in_solution.insert(ustar);
                tabu_list[ustar] = iter;
            }
        }
        if (solution_weight_sum <= g->C && solution_profit_sum > best_solution_profit_sum)
        {
            Solution_To_Best_Solution();
        }
        if (solution_weight_sum >= g->C)
        {
            int ustar = -1;
            int ustar_idx;
            for (int i = 0; i < in_solution.size(); i++)
            {
                int item = in_solution[i];
                if (block_list[item]) continue;
                if (iter - tabu_list[item] < rand_deviation(tabu_length)) continue;
                if (ustar == -1)
                {
                    ustar = item;
                    ustar_idx = i;
                    continue;
                }
                if (solution_contribution[item] * g->weight[ustar] <
                    solution_contribution[ustar] * g->weight[item])
                {
                    ustar = item;
                    ustar_idx = i;
                }
            }
            if (ustar != -1)
            {
                Remove_Item(ustar);
                in_solution.erase(ustar_idx);
                tabu_list[ustar] = iter;
            }
        }
        if (solution_weight_sum <= g->C && solution_profit_sum > best_solution_profit_sum)
        {
            Solution_To_Best_Solution();
        }
        iter++;
    }

    //recalculate conf_change
    for (int i = 1; i <= g->m; i++)
    {
        if (solution[i])
        {
            conf_change_in_solution[i] = 0;
            for (int elem_nei: g->item_neighbor[i])
            {
                if (solution_elements[elem_nei] == 1)
                    conf_change_in_solution[i]++;
            }
            origin_conf_change_in_solution[i] = conf_change_in_solution[i];
        }
        else
        {
            conf_change_out_of_solution[i] = 0;
            for (int elem_nei: g->item_neighbor[i])
            {
                if (solution_elements[elem_nei] == 0)
                    conf_change_out_of_solution[i]++;
            }
            origin_conf_change_out_of_solution[i] = conf_change_out_of_solution[i];
        }
    }
}

void BMCP::BMCPSolver::Start_Clock()
{
    start_time = clock();
}

int BMCP::BMCPSolver::Get_Time()
{
    now_time = clock();
    return now_time - start_time;
}

void BMCP::BMCPSolver::Solve()
{
    Start_Clock();
    Greedy_Initialization();
    while (Get_Time() < time_limit * CLOCKS_PER_SEC)
    {
        if (solution_profit_sum > star_solution_profit_sum) [[unlikely]]
        {
            //star_solution_time = Get_Time();
            Solution_To_Star_Solution();
            printf("%lf %d %d\n", 1.0 * Get_Time() / CLOCKS_PER_SEC, total_iterations, star_solution_profit_sum);
            fflush(stdout);
        }
        total_iterations++;
        CC_Search();
        Best_Solution_To_Solution();
        if (solution_profit_sum > star_solution_profit_sum) [[unlikely]]
        {
            //star_solution_time = Get_Time();
            Solution_To_Star_Solution();
            printf("%lf %d %d\n", 1.0 * Get_Time() / CLOCKS_PER_SEC, total_iterations, star_solution_profit_sum);
            fflush(stdout);
        }
        Deep_Optimize();
        Best_Solution_To_Solution();
        //printf("%lf %d %d\n", 1.0 * Get_Time() / CLOCKS_PER_SEC, total_iterations, star_solution_profit_sum);
    }
}

int BMCP::BMCPSolver::rand_deviation(int num)
{
    return num + (rand() % (int) (0.3 * num) - (int) 0.15 * num);
}