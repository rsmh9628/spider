#pragma once

#include <queue>
#include <rack.hpp>

namespace ph {

enum class ToggleEdgeResult { ADDED, REMOVED, SWAPPED, CYCLE };

template <typename T>
class DirectedGraph {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

public:
    DirectedGraph(int vertices)
        : adjList(vertices)
        , reverseAdjList(vertices) {}

    ToggleEdgeResult addEdge(T src, T dest) {
        adjList[src].push_back(dest);
        reverseAdjList[dest].push_back(src);

        if (isCyclic()) {
            adjList[src].pop_back();
            reverseAdjList[dest].pop_back();
            return ToggleEdgeResult::CYCLE;
        }

        return ToggleEdgeResult::ADDED;
    }

    ToggleEdgeResult toggleEdge(T src, T dest) {
        if (hasEdge(src, dest)) {
            removeEdge(src, dest);
            return ToggleEdgeResult::REMOVED;
        } else if (hasEdge(dest, src)) {
            removeEdge(dest, src);
            if (addEdge(src, dest) == ToggleEdgeResult::ADDED) {
                return ToggleEdgeResult::SWAPPED;
            } else {
                addEdge(dest, src);
                return ToggleEdgeResult::CYCLE;
            }
        } else {
            return addEdge(src, dest);
        }
    }

    void removeEdge(T src, T dest) {
        auto& list = adjList[src];
        list.erase(std::remove(list.begin(), list.end(), dest), list.end());

        auto& reverseList = reverseAdjList[dest];
        reverseList.erase(std::remove(reverseList.begin(), reverseList.end(), src), reverseList.end());
    }

    bool detectCycle(T vertex, std::vector<bool>& visited, std::vector<bool>& recStack) {
        if (!visited[vertex]) {
            visited[vertex] = true;
            recStack[vertex] = true;

            for (const auto& dest : adjList[vertex]) {
                if (!visited[dest] && detectCycle(dest, visited, recStack)) {
                    return true;
                } else if (recStack[dest]) {
                    return true;
                }
            }
        }

        recStack[vertex] = false;
        return false;
    }

    bool isCyclic() {
        std::vector<bool> visited(adjList.size(), false);
        std::vector<bool> recStack(adjList.size(), false);

        for (size_t i = 0; i < adjList.size(); i++) {
            if (detectCycle(i, visited, recStack)) {
                return true;
            }
        }
        return false;
    }

    std::vector<T> topologicalSort() const {
        std::vector<T> incomingEdgesCount(adjList.size(), 0);

        for (const auto& list : adjList) {
            for (const auto& dest : list) {
                incomingEdgesCount[dest]++;
            }
        }

        std::queue<T> q;
        for (size_t i = 0; i < adjList.size(); i++) {
            if (incomingEdgesCount[i] == 0) {
                q.push(i);
            }
        }

        std::vector<T> result;
        while (!q.empty()) {
            T u = q.front();
            q.pop();
            result.push_back(u);

            for (const auto& dest : adjList[u]) {
                if (--incomingEdgesCount[dest] == 0) {
                    q.push(dest);
                }
            }
        }

        return result;
    }

    const std::vector<T>& getAdjacentVertices(T vertex) const { return adjList[vertex]; }
    const std::vector<T>& getAdjacentVerticesRev(T vertex) const { return reverseAdjList[vertex]; }

    bool hasEdge(T src, T dest) const {
        return std::find(adjList[src].begin(), adjList[src].end(), dest) != adjList[src].end();
    }

    bool hasReverseEdge(T src, T dest) const {
        return std::find(reverseAdjList[src].begin(), reverseAdjList[src].end(), dest) != reverseAdjList[src].end();
    }

    json_t* toJson() const {
        json_t* rootJ = json_array();

        for (const auto& list : adjList) {
            json_t* sublistJ = json_array();
            for (const auto& dest : list) {
                json_array_append_new(sublistJ, json_integer(dest));
            }
            json_array_append_new(rootJ, sublistJ);
        }

        return rootJ;
    }

    void fromJson(json_t* rootJ) {
        size_t size = json_array_size(rootJ);
        adjList.resize(size);
        reverseAdjList.resize(size);

        for (size_t i = 0; i < size; ++i) {
            json_t* sublistJ = json_array_get(rootJ, i);
            size_t sublistSize = json_array_size(sublistJ);
            for (size_t j = 0; j < sublistSize; ++j) {
                adjList[i].push_back(json_integer_value(json_array_get(sublistJ, j)));
                reverseAdjList[json_integer_value(json_array_get(sublistJ, j))].push_back(i);
            }
        }
    }

    void clear() {
        for (auto& list : adjList) {
            list.clear();
        }
        for (auto& list : reverseAdjList) {
            list.clear();
        }
    }

    std::vector<std::vector<T>> adjList;
    std::vector<std::vector<T>> reverseAdjList;
};

} // namespace ph