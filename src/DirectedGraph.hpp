#pragma once

#include <queue>

namespace ph {

enum class ToggleEdgeResult { ADDED, REMOVED, REMOVED_REV, CYCLE };

template <typename T>
class DirectedGraph {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

private:
    std::vector<std::vector<T>> adjList;
    std::vector<std::vector<T>> reverseAdjList;

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
            return ToggleEdgeResult::REMOVED_REV;
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

    bool isCyclicHelper(T vertex, std::vector<bool>& visited, std::vector<bool>& recStack) {
        if (!visited[vertex]) {
            visited[vertex] = true;
            recStack[vertex] = true;

            for (const auto& dest : adjList[vertex]) {
                if (!visited[dest] && isCyclicHelper(dest, visited, recStack)) {
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

        for (T i = 0; i < adjList.size(); i++) {
            if (isCyclicHelper(i, visited, recStack)) {
                return true;
            }
        }
        return false;
    }

    std::vector<T> topologicalSort() {
        std::vector<T> inDegree(adjList.size(), 0);

        for (const auto& list : adjList) {
            for (const auto& dest : list) {
                inDegree[dest]++;
            }
        }

        std::queue<T> q;
        for (T i = 0; i < adjList.size(); i++) {
            if (inDegree[i] == 0) {
                q.push(i);
            }
        }

        std::vector<T> result;
        while (!q.empty()) {
            T u = q.front();
            q.pop();
            result.push_back(u);

            for (const auto& dest : adjList[u]) {
                if (--inDegree[dest] == 0) {
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
};

} // namespace ph