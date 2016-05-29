#pragma once

#include "gtest/gtest.h"
#include "sg.h"

#include <map>
#include <cstdint>
#include <thread>
#include <mutex>
#include <random>

class ParameterFactory : public Gravity::DefaultSceneGraph::ParameterFactory
{
public:
    std::map<std::string, Gravity::Parameter> GetParameterSet(std::uint32_t const &type) const override
    {
        switch (type)
        {
            case 0:
            default:
            {
                std::map<std::string, Gravity::Parameter> params;
                params.emplace("type", 5);
                params.emplace("float_value", 3.8f);
                params.emplace("vector_value", std::vector<int>{1,2,3});
                return params;
            }

        }
    }
};

// Api creation fixture, prepares api_ for further tests
class App : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        m_sg = Gravity::CreateDefaultSceneGraph(new ParameterFactory);
    }

    virtual void TearDown()
    {
        delete m_sg;
    }

    // Scene graph
    Gravity::DefaultSceneGraph *m_sg;
};

TEST_F(App, Parameter_SimpleTypes)
{
    Gravity::Parameter p = 5;

    ASSERT_EQ(p.As<int>(), 5);
    ASSERT_EQ(p.As < int const>(), 5);
    ASSERT_EQ(p.As<int &&>(), 5);

    p = 55;

    ASSERT_EQ(p.As<int>(), 55);
    ASSERT_EQ(p.As < int const>(), 55);
    ASSERT_EQ(p.As<int &&>(), 55);

    p = 3.7f;

    ASSERT_EQ(p.As<float>(), 3.7f);
    ASSERT_EQ(p.As < float const>(), 3.7f);
    ASSERT_EQ(p.As<float &&>(), 3.7f);
}

TEST_F(App, Parameter_ComplexTypes)
{
    Gravity::Parameter p;

    p = std::vector<int>{1, 2, 3};

    ASSERT_EQ(p.As<std::vector<int>>(), (std::vector<int>{1, 2, 3}));
}

TEST_F(App, Parameter_ModifyValue)
{
    // Сallback counters
    int create_count = 0;
    int delete_count = 0;
    int update_count = 0;

    std::set<std::uint32_t> filters = {0, 1, 2};

    // Setup callbacks
    ASSERT_NO_THROW(m_sg->RegisterOnNodeCreateCallback(
            [&create_count](Gravity::DefaultSceneGraph::Node *node)
            { ++create_count; }, filters));
    ASSERT_NO_THROW(m_sg->RegisterOnNodeDeleteCallback(
            [&delete_count](Gravity::DefaultSceneGraph::Node *node)
            { ++delete_count; }, filters));
    ASSERT_NO_THROW(m_sg->RegisterOnNodeParameterChangeCallback(
            [&update_count](Gravity::DefaultSceneGraph::Node *node, const std::string &key)
            { ++update_count; },
            filters));

    auto node = m_sg->CreateNode(0);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(create_count, 1);
    ASSERT_NO_THROW(node->SetValue("type", std::vector<int>{1, 2, 3}));
    ASSERT_EQ(update_count, 1);

    ASSERT_NO_THROW(node->ModifyValue<std::vector<int>>("type",
                                                        [](std::vector<int> &val)
                                                        {
                                                            val.push_back(4);
                                                            val.push_back(5);
                                                        }
    ));
    ASSERT_EQ(update_count, 2);

    auto val = node->GetValue<std::vector<int>>("type");
    auto ref = std::vector<int>{1,2,3,4,5};
    ASSERT_EQ(val, ref);

    ASSERT_NO_THROW(m_sg->DeleteNode(node));
    node = nullptr;
    ASSERT_EQ(delete_count, 1);
}



TEST_F(App, SceneGraph_Node)
{
    auto node = m_sg->CreateNode(0);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->GetType(), 0);

    ASSERT_NO_THROW(m_sg->DeleteNode(node));
    ASSERT_ANY_THROW(m_sg->DeleteNode(node));
}

TEST_F(App, SceneGraph_SetValue)
{
    auto node = m_sg->CreateNode(0);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->GetType(), 0);

    ASSERT_NO_THROW(node->SetValue("type", 10));
    ASSERT_NO_THROW(node->SetValue("float_value", 5.f));
}

TEST_F(App, SceneGraph_Callback)
{
    // Callbacks fired counters
    int create_count = 0;
    int delete_count = 0;
    int update_count = 0;

    // Setup callbacks
    ASSERT_NO_THROW(m_sg->RegisterOnNodeCreateCallback(
            [&create_count](Gravity::DefaultSceneGraph::Node *node)
            { ++create_count; }));
    ASSERT_NO_THROW(m_sg->RegisterOnNodeDeleteCallback(
            [&delete_count](Gravity::DefaultSceneGraph::Node *node)
            { ++delete_count; }));
    ASSERT_NO_THROW(m_sg->RegisterOnNodeParameterChangeCallback(
            [&update_count](Gravity::DefaultSceneGraph::Node *node, const std::string &key)
            { ++update_count; }));

    auto node = m_sg->CreateNode(0);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(create_count, 1);
    ASSERT_NO_THROW(node->SetValue("type", 10));
    ASSERT_EQ(update_count, 1);
    ASSERT_NO_THROW(m_sg->DeleteNode(node));
    node = nullptr;
    ASSERT_EQ(delete_count, 1);
}

TEST_F(App, SceneGraph_Callback_Filter)
{

    // Callbacks counters
    int create_count = 0;
    int delete_count = 0;
    int update_count = 0;

    std::set<std::uint32_t> filters = {0, 1};

    // Setup callbacks
    ASSERT_NO_THROW(m_sg->RegisterOnNodeCreateCallback(
            [&create_count](Gravity::DefaultSceneGraph::Node *node)
            { ++create_count; }, filters));
    ASSERT_NO_THROW(m_sg->RegisterOnNodeDeleteCallback(
            [&delete_count](Gravity::DefaultSceneGraph::Node *node)
            { ++delete_count; }, filters));
    ASSERT_NO_THROW(m_sg->RegisterOnNodeParameterChangeCallback(
            [&update_count](Gravity::DefaultSceneGraph::Node *node, const std::string &key)
            { ++update_count; },
            filters));

    // Not in the filter
    auto node = m_sg->CreateNode(2);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(create_count, 0);
    ASSERT_NO_THROW(node->SetValue("type", 10));
    ASSERT_EQ(update_count, 0);
    ASSERT_NO_THROW(m_sg->DeleteNode(node));
    node = nullptr;
    ASSERT_EQ(delete_count, 0);

    // In the filter
    node = m_sg->CreateNode(0);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(create_count, 1);
    ASSERT_NO_THROW(node->SetValue("type", 10));
    ASSERT_EQ(update_count, 1);
    ASSERT_NO_THROW(m_sg->DeleteNode(node));
    node = nullptr;
    ASSERT_EQ(delete_count, 1);
}

TEST_F(App, SceneGraph_Node_GetValue)
{
    // Not in the filter
    auto node = m_sg->CreateNode(2);
    ASSERT_NE(node, nullptr);

    ASSERT_NO_THROW(node->SetValue("type", 10));
    ASSERT_EQ(node->GetValue<int>("type"), 10);
    ASSERT_EQ(node->GetValue<int&&>("type"), 10);
    ASSERT_EQ(node->GetValue<int const>("type"), 10);
    ASSERT_EQ(node->GetValue<int const&>("type"), 10);

    auto val = std::vector<int>{4, 5, 6};
    ASSERT_NO_THROW(node->SetValue("vector_value", val));
    ASSERT_EQ(node->GetValue<std::vector<int>>("vector_value"), val);

    ASSERT_NO_THROW(m_sg->DeleteNode(node));
}

TEST_F(App, SceneGraph_Multithreaded_Consistency)
{
    // Number of threads
    int const kNumThreads = std::thread::hardware_concurrency();
    // Number of nodes to create per thread
    int const kNumNodesPerThread = 100;
    std::vector<std::thread> threads;

    // Nodes are stored here
    std::vector<Gravity::DefaultSceneGraph::Node*> nodes;
    // Mutex to guard concurrent access to nodes array
    std::mutex nodes_mutex;

    // Start threads to concurrently create nodes
    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.push_back(std::thread([this, &nodes_mutex, &nodes, kNumNodesPerThread]()
                                 {
                                     for (int j = 0; j < kNumNodesPerThread; ++j)
                                     {
                                         auto node = m_sg->CreateNode(0);

                                         {
                                             std::unique_lock<std::mutex> lock(nodes_mutex);

                                             nodes.push_back(node);
                                         }
                                     }
                                 }));
    }

    // Join all threads
    for (int i = 0; i < kNumThreads; ++i)
    {
        threads[i].join();
    }

    threads.clear();

    // Now create threads to randomly pick nodes and change node parameters concurrently
    std::default_random_engine rng;
    std::uniform_int_distribution<int> dist;
    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.push_back(std::thread([this, &nodes, &rng, &dist, kNumNodesPerThread, kNumThreads]()
                                      {
                                          for (int j = 0; j < kNumNodesPerThread; ++j)
                                          {
                                              int idx = dist(rng) % kNumThreads * kNumNodesPerThread;
                                              auto node = nodes[idx];

                                              node->SetValue("type", dist(rng));
                                          }
                                      }));
    }

    // Join all threads
    for (int i = 0; i < kNumThreads; ++i)
    {
        threads[i].join();
    }

    threads.clear();

    // Now start threads to concurrently delete all the nodes
    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.push_back(std::thread([this, &nodes_mutex, &nodes, &rng, &dist, kNumNodesPerThread]()
                                      {
                                          for (int j = 0; j < kNumNodesPerThread; ++j)
                                          {
                                              Gravity::DefaultSceneGraph::Node* node = nullptr;
                                              {
                                                  std::unique_lock<std::mutex> lock(nodes_mutex);

                                                  int idx = dist(rng) % nodes.size();

                                                  node = nodes[idx];

                                                  auto iter = nodes.cbegin() + idx;

                                                  nodes.erase(iter);
                                              }

                                              m_sg->DeleteNode(node);
                                          }
                                      }));
    }

    // Join all threads
    for (int i = 0; i < kNumThreads; ++i)
    {
        threads[i].join();
    }
}


