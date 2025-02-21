#include <catch2/catch_all.hpp>
#include "../src/DirectedGraph.hpp"

using namespace Catch;
using namespace Catch::Matchers;
using namespace Catch::Generators;

namespace ph {

TEST_CASE("6 vertex graph has 6 vertices and empty edge sets", "[DirectedGraph]") {
    DirectedGraph<int> graph(6);
    REQUIRE(graph.adjList.size() == 6);
    REQUIRE(graph.reverseAdjList.size() == 6);

    for (int i = 0; i < 6; ++i) {
        REQUIRE(graph.adjList[i].empty());
        REQUIRE(graph.reverseAdjList[i].empty());
    }
}

TEST_CASE("Adding an edge updates adjacency lists", "[DirectedGraph]") {
    DirectedGraph<int> graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(0, 2);
    graph.addEdge(1, 2);

    REQUIRE(graph.adjList[0] == std::vector<int>{1, 2});
    REQUIRE(graph.adjList[1] == std::vector<int>{2});
    REQUIRE(graph.adjList[2].empty());

    REQUIRE(graph.reverseAdjList[0].empty());
    REQUIRE(graph.reverseAdjList[1] == std::vector<int>{0});
    REQUIRE(graph.reverseAdjList[2] == std::vector<int>{0, 1});
}

TEST_CASE("Removing an edge updates adjacency lists", "[DirectedGraph]") {
    DirectedGraph<int> graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(0, 2);
    graph.addEdge(1, 2);

    graph.removeEdge(0, 1);
    graph.removeEdge(1, 2);

    REQUIRE(graph.adjList[0] == std::vector<int>{2});
    REQUIRE(graph.adjList[1].empty());
    REQUIRE(graph.adjList[2].empty());

    REQUIRE(graph.reverseAdjList[0].empty());
    REQUIRE(graph.reverseAdjList[1].empty());
    REQUIRE(graph.reverseAdjList[2] == std::vector<int>{0});
}

TEST_CASE("Graph detects a cycle correctly", "[DirectedGraph]") {
    DirectedGraph<int> graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    // addEdge should detect a cycle itself so we add one manually
    // there is another test case for testing addEdge detecting it correctly
    graph.adjList[2].push_back(0);
    graph.reverseAdjList[0].push_back(2);

    REQUIRE(graph.isCyclic());
}

TEST_CASE("Graph detects acyclic graph correctly", "[DirectedGraph]") {
    DirectedGraph<int> graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    REQUIRE_FALSE(graph.isCyclic());
}

TEST_CASE("Topological sort of a graph with no edges is the order of the vertices", "[DirectedGraph]") {
    DirectedGraph<int> graph(3);
    auto order = graph.topologicalSort();
    REQUIRE(order == std::vector<int>{0, 1, 2});
}

TEST_CASE("Topological sort of vertices in order is the correct topological order", "[DirectedGraph]") {
    DirectedGraph<int> graph(6);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(3, 4);
    graph.addEdge(4, 5);

    auto order = graph.topologicalSort();
    REQUIRE(order == std::vector<int>{0, 1, 2, 3, 4, 5});
}

TEST_CASE("Topological sort with multiple correct orders returns one of the correct orders", "[DirectedGraph]") {
    DirectedGraph<int> graph(6);

    graph.addEdge(0, 2);
    graph.addEdge(0, 3);
    graph.addEdge(1, 3);
    graph.addEdge(1, 4);
    graph.addEdge(2, 5);
    graph.addEdge(3, 5);
    graph.addEdge(4, 5);

    auto order = graph.topologicalSort();

    // multiple valid sorts
    REQUIRE((order == std::vector<int>{0, 1, 2, 3, 4, 5} || order == std::vector<int>{0, 1, 3, 2, 4, 5} ||
             order == std::vector<int>{1, 0, 3, 2, 4, 5} || order == std::vector<int>{1, 0, 2, 3, 4, 5}));
}

TEST_CASE("Adding an edge that creates a cycle fails", "[DirectedGraph]") {
    DirectedGraph<int> graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    auto result = graph.addEdge(2, 0);
    REQUIRE(result == ToggleEdgeResult::CYCLE);

    REQUIRE(graph.adjList[0] == std::vector<int>{1});
    REQUIRE(graph.adjList[1] == std::vector<int>{2});
    REQUIRE(graph.adjList[2] == std::vector<int>{});

    REQUIRE(graph.reverseAdjList[0] == std::vector<int>{});
    REQUIRE(graph.reverseAdjList[1] == std::vector<int>{0});
    REQUIRE(graph.reverseAdjList[2] == std::vector<int>{1});
}

TEST_CASE("Toggling an edge that creates a cycle fails", "[DirectedGraph]") {
    DirectedGraph<int> graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(0, 2);

    auto result = graph.toggleEdge(2, 0);

    REQUIRE(graph.adjList[0] == std::vector<int>{1, 2});
    REQUIRE(graph.adjList[1] == std::vector<int>{2});
    REQUIRE(graph.adjList[2] == std::vector<int>{});

    REQUIRE(graph.reverseAdjList[0] == std::vector<int>{});
    REQUIRE(graph.reverseAdjList[1] == std::vector<int>{0});
    REQUIRE(graph.reverseAdjList[2] == std::vector<int>{1, 0});
}

TEST_CASE("Toggling an edge whose reverse exists swaps the edge", "[DirectedGraph]") {
    DirectedGraph<int> graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    auto result = graph.toggleEdge(2, 1);
    REQUIRE(result == ToggleEdgeResult::SWAPPED);

    REQUIRE(graph.adjList[0] == std::vector<int>{1});
    REQUIRE(graph.adjList[1] == std::vector<int>{});
    REQUIRE(graph.adjList[2] == std::vector<int>{1});

    REQUIRE(graph.reverseAdjList[0] == std::vector<int>{});
    REQUIRE(graph.reverseAdjList[1] == std::vector<int>{0, 2});
    REQUIRE(graph.reverseAdjList[2] == std::vector<int>{});
}

TEST_CASE("Has edge returns true if edge exists", "[DirectedGraph]") {
    DirectedGraph<int> graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    REQUIRE(graph.hasEdge(0, 1));
    REQUIRE(graph.hasEdge(1, 2));
    REQUIRE_FALSE(graph.hasEdge(0, 2));
}

TEST_CASE("Has reverse edge returns true if edge exists", "[DirectedGraph]") {
    DirectedGraph<int> graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);

    REQUIRE(graph.hasReverseEdge(1, 0));
    REQUIRE(graph.hasReverseEdge(2, 1));
    REQUIRE_FALSE(graph.hasReverseEdge(2, 0));
}

} // namespace ph