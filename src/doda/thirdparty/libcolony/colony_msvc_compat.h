/*
 * MIT License
 *
 * Copyright (c) 2023 Marek Rogalski
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Upstream: https://github.com/mafik/libcolony
 * Pinned commit: 7406bd9fbb00e53273ea9da84eeba5aa26af382f
 *
 * DoDA compatibility adaptation:
 * VLA temporary storage only was replaced with std::vector because MSVC C++ does not support C99 VLAs.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace colony {

// This is a helper function that computes the cost of a task taking into
// account travel time, work time, risk of retry & priority.
//
// `travel_time` should be the time it takes to get to the task location (>=0)
// `work_time` should be the time it takes to execute the task (>=0)
// `retry_risk` should be the probability of failure [0,1)
// `priority` should be the priority of the task (>0)
//
// Note that computing cost of impossible tasks (with retry risk of 1 or
// priority of 0) doesn't make sense and will return infinity.
inline double ComputeCost(double travel_time = 0, double work_time = 0,
                          double retry_risk = 0, double priority = 1) {
  if (retry_risk >= 1 || priority <= 0) {
    return std::numeric_limits<double>::infinity();
  }
  double cost = travel_time + work_time;
  cost /= 1.0 - retry_risk;
  cost /= priority;
  return cost;
}

typedef int CharacterId;
typedef int TaskId;

struct Assignment {
  CharacterId character;
  TaskId task;
  double cost;
};

// Reduce the number of potential assignments for each character & task.
inline void LimitAssignments(std::vector<Assignment> &assignments,
                             int limit_per_character, int limit_per_task) {
  std::sort(
      assignments.begin(), assignments.end(),
      [](const Assignment &a, const Assignment &b) { return a.cost < b.cost; });
  CharacterId last_character = -1;
  TaskId last_task = -1;
  for (auto &a : assignments) {
    last_character = std::max(last_character, a.character);
    last_task = std::max(last_task, a.task);
  }
  if (last_character < 0 || last_task < 0) {
    return;
  }
  std::vector<int> tasks_per_character(last_character + 1, 0);
  std::vector<int> characters_per_task(last_task + 1, 0);

  for (int i = 0; i < static_cast<int>(assignments.size()); ++i) {
    auto &a = assignments[i];
    if (tasks_per_character[a.character] >= limit_per_character) {
      std::swap(a, assignments.back());
      assignments.pop_back();
      --i;
      continue;
    }
    if (characters_per_task[a.task] >= limit_per_task) {
      std::swap(a, assignments.back());
      assignments.pop_back();
      --i;
      continue;
    }
    ++tasks_per_character[a.character];
    ++characters_per_task[a.task];
  }
}

// The main function of this library. It takes a vector of potential assigments
// of characters to tasks and removes all assignments that are not optimal.
inline void Optimize(std::vector<Assignment> &assignments) {
  if (assignments.empty()) {
    return;
  }

  // Convert the problem into max-value assignment.
  CharacterId max_character = 0;
  TaskId max_task = 0;
  double max_cost = 0;
  for (auto &a : assignments) {
    max_character = std::max(max_character, a.character);
    max_task = std::max(max_task, a.task);
    max_cost = std::max(max_cost, a.cost);
  }

  int NX, NY;

  // The algorithm finds the optimal assignment only when NX <= NY.
  if (max_task > max_character) {
    NX = max_character + 1;
    NY = max_task + 1;
  } else {
    NX = max_task + 1;
    NY = max_character + 1;
  }

  std::vector<std::vector<double>> value(NX, std::vector<double>(NY, 1.0));

  if (max_task > max_character) {
    for (auto &a : assignments) {
      value[a.character][a.task] = max_cost - a.cost + 1.0;
    }
  } else {
    for (auto &a : assignments) {
      value[a.task][a.character] = max_cost - a.cost + 1.0;
    }
  }

  int max_match = 0;                     // n workers and n jobs
  std::vector<double> lx(NX, 0.0);       // labels of X and Y parts
  std::vector<double> ly(NY, 0.0);
  std::vector<int> xy(NX, -1);           // xy[x] - vertex that is matched with x,
  std::vector<int> yx(NY, -1);           // yx[y] - vertex that is matched with y
  std::vector<uint8_t> S(NX, 0);         // sets S and T in algorithm
  std::vector<uint8_t> T(NY, 0);
  std::vector<double> slack(NY, 0.0);    // as in the algorithm description
  std::vector<int> slackx(NY, 0);        // slackx[y] such a vertex, that
  // l(slackx[y]) + l(y) - w(slackx[y],y) = slack[y]
  std::vector<int> prev(NX, -1);         // array for memorizing alternating paths

  auto update_labels = [&]() {
    int x, y;
    double delta = std::numeric_limits<double>::max();
    for (y = 0; y < NY; y++) // calculate delta using slack
      if (!T[y])
        delta = std::min(delta, slack[y]);
    for (x = 0; x < NX; x++) // update X labels
      if (S[x])
        lx[x] -= delta;
    for (y = 0; y < NY; y++) // update Y labels
      if (T[y])
        ly[y] += delta;
    for (y = 0; y < NY; y++) // update slack array
      if (!T[y])
        slack[y] -= delta;
  };

  // x - current vertex,prevx - vertex from X before x in the alternating
  // path, so we add edges (prevx, xy[x]), (xy[x], x)
  auto add_to_tree = [&](int x, int prevx) {
    S[x] = 1;        // add x to S
    prev[x] = prevx; // we need this when augmenting
    for (int y = 0; y < NY;
         y++) // update slacks, because we add new vertex to S
      if (lx[x] + ly[y] - value[x][y] < slack[y]) {
        slack[y] = lx[x] + ly[y] - value[x][y];
        slackx[y] = x;
      }
  };

  for (int x = 0; x < NX; x++)
    for (int y = 0; y < NY; y++)
      lx[x] = std::max(lx[x], value[x][y]);

  auto eq = [](double a, double b) { return std::abs(a - b) < 0.0001; };

  while (max_match < std::min(NX, NY)) {
    int x = 0, y = 0, root = 0; // just counters and root vertex
    std::vector<int> q(NX, 0);
    int wr = 0, rd = 0; // q - queue for bfs, wr,rd - write and read pos in queue
    std::fill(S.begin(), S.end(), 0); // init set S
    std::fill(T.begin(), T.end(), 0); // init set T
    std::fill(prev.begin(), prev.end(), -1); // init set prev - for the alternating tree
    double best_lx = std::numeric_limits<double>::min();
    for (x = 0; x < NX; x++) // finding root of the tree
      if (xy[x] == -1) {
        if (lx[x] > best_lx) {
          best_lx = lx[x];
          root = x;
        }
      }

    q[wr++] = root;
    prev[root] = -2;
    S[root] = 1;

    for (y = 0; y < NY; y++) { // initializing slack array
      slack[y] = lx[root] + ly[y] - value[root][y];
      slackx[y] = root;
    }

    while (true) {               // main cycle
      while (rd < wr) {          // building tree with bfs cycle
        x = q[rd++];             // current vertex from X part
        for (y = 0; y < NY; y++) // iterate through all edges in equality
                                 // graph
          if (eq(value[x][y], lx[x] + ly[y]) && !T[y]) {
            if (yx[y] == -1)
              break;         // an exposed vertex in Y found, so augmenting path
                             // exists!
            T[y] = 1;        // else just add y to T,
            q[wr++] = yx[y]; // add vertex yx[y], which is matched with y, to
                             // the queue
            add_to_tree(yx[y],
                        x); // add edges (x,y) and (y,yx[y]) to the tree
          }
        if (y < NY)
          break; // augmenting path found!
      }
      if (y < NY)
        break; // augmenting path found!

      update_labels(); // augmenting path not found, so improve labeling
      wr = rd = 0;
      for (y = 0; y < NY; y++)
        // in this cycle we add edges that were added to the equality graph as
        // a result of improving the labeling, we add edge (slackx[y], y) to
        // the tree if and only if !T[y] && slack[y] == 0, also with this edge
        // we add another one (y, yx[y]) or augment the matching, if y was
        // exposed
        if (!T[y] && eq(slack[y], 0)) {
          if (yx[y] ==
              -1) { // exposed vertex in Y found - augmenting path exists!
            x = slackx[y];
            break;
          } else {
            T[y] = 1; // else just add y to T,
            if (!S[yx[y]]) {
              q[wr++] = yx[y]; // add vertex yx[y], which is matched with
              // y, to the queue
              add_to_tree(yx[y], slackx[y]); // and add edges (x,y) and (y,
              // yx[y]) to the tree
            }
          }
        }
      if (y < NY)
        break; // augmenting path found!
    }

    if (y < NY) {  // we found augmenting path!
      max_match++; // increment matching
      // in this cycle we inverse edges along augmenting path
      for (int cx = x, cy = y, ty = 0; cx != -2; cx = prev[cx], cy = ty) {
        ty = xy[cx];
        yx[cy] = cx;
        xy[cx] = cy;
      }
    }
  }

  if (max_task > max_character) {
    for (int i = 0; i < static_cast<int>(assignments.size()); ++i) {
      if (assignments[i].task != xy[assignments[i].character]) {
        std::swap(assignments[i], assignments.back());
        assignments.pop_back();
        --i;
      }
    }
  } else {
    for (int i = 0; i < static_cast<int>(assignments.size()); ++i) {
      if (assignments[i].task != yx[assignments[i].character]) {
        std::swap(assignments[i], assignments.back());
        assignments.pop_back();
        --i;
      }
    }
  }
}

} // namespace colony
