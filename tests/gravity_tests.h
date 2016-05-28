#pragma once
#include "gtest/gtest.h"
#include "sg.h"

#include <map>
#include <cstdint>

class ParameterFactory : public Gravity::DefaultSceneGraph::ParameterFactory
{
public:
    std::map<std::string, Gravity::Parameter> GetParameterSet(std::uint32_t const& type) const override
    {
        switch (type)
        {
            case 0:
            default:
            {
                std::map<std::string, Gravity::Parameter> params;
                params.emplace("type", 5);
                params.emplace("float_value", 3.8f);
                return params;
            }

        }
    }
};

// Api creation fixture, prepares api_ for further tests
class App : public ::testing::Test {
public:
    virtual void SetUp() {
        m_sg = new Gravity::DefaultSceneGraph(new ParameterFactory);
    }

    virtual void TearDown() {
        delete m_sg;
    }

    Gravity::DefaultSceneGraph *m_sg;
};


//check create/delete node functionality
TEST_F(App, SceneGraph_Node) {
    auto node = m_sg->CreateNode(0);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->GetType(), 0);

    ASSERT_NO_THROW(m_sg->DeleteNode(node));
    ASSERT_ANY_THROW(m_sg->DeleteNode(node)); // node isn't valid anymore
}

//check SetValue works properly
TEST_F(App, SceneGraph_SetValue) {
    auto node = m_sg->CreateNode(0);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->GetType(), 0);

    ASSERT_NO_THROW(node->SetValue("type", 10));
    ASSERT_NO_THROW(node->SetValue("float_value", 5.f));
}

//check SetValue works properly
TEST_F(App, SceneGraph_Callback) {
//callbacks fired counters
    int create_count = 0;
    int delete_count = 0;
    int update_count = 0;

//setup callbacks
    ASSERT_NO_THROW(m_sg->RegisterOnNodeCreateCallback(
            [&create_count](Gravity::DefaultSceneGraph::Node *node) { ++create_count; }));
    ASSERT_NO_THROW(m_sg->RegisterOnNodeDeleteCallback(
            [&delete_count](Gravity::DefaultSceneGraph::Node *node) { ++delete_count; }));
    ASSERT_NO_THROW(m_sg->RegisterOnNodeParameterChangeCallback(
            [&update_count](Gravity::DefaultSceneGraph::Node *node, const std::string &key) { ++update_count; }));

//checks
    auto node = m_sg->CreateNode(0);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(create_count, 1);
    ASSERT_NO_THROW(node->SetValue("type", 10));
    ASSERT_EQ(update_count, 1);
    ASSERT_NO_THROW(m_sg->DeleteNode(node));
    node = nullptr;
    ASSERT_EQ(delete_count, 1);
}

TEST_F(App, SceneGraph_Callback_Filter) {

//callbacks fired counters
    int create_count = 0;
    int delete_count = 0;
    int update_count = 0;

    std::set<std::uint32_t> filters = {0, 1};
//setup callbacks
    ASSERT_NO_THROW(m_sg->RegisterOnNodeCreateCallback(
            [&create_count](Gravity::DefaultSceneGraph::Node *node) { ++create_count; }, filters));
    ASSERT_NO_THROW(m_sg->RegisterOnNodeDeleteCallback(
            [&delete_count](Gravity::DefaultSceneGraph::Node *node) { ++delete_count; }, filters));
    ASSERT_NO_THROW(m_sg->RegisterOnNodeParameterChangeCallback(
            [&update_count](Gravity::DefaultSceneGraph::Node *node, const std::string &key) { ++update_count; },
            filters));

//checks
//not in filter
    auto node = m_sg->CreateNode(2);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(create_count, 0);
    ASSERT_NO_THROW(node->SetValue("type", 10));
    ASSERT_EQ(update_count, 0);
    ASSERT_NO_THROW(m_sg->DeleteNode(node));
    node = nullptr;
    ASSERT_EQ(delete_count, 0);

    node = m_sg->CreateNode(0);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(create_count, 1);
    ASSERT_NO_THROW(node->SetValue("type", 10));
    ASSERT_EQ(update_count, 1);
    ASSERT_NO_THROW(m_sg->DeleteNode(node));
    node = nullptr;
    ASSERT_EQ(delete_count, 1);
}

TEST_F(App, UniversalParameter_SimpleTypes) {
    Gravity::Parameter p = 5;

    ASSERT_EQ(p.As<int>(), 5);
    ASSERT_EQ(p.As < int const>(), 5);
    ASSERT_EQ(p.As<int &&>(), 5);

    p = 55;

    ASSERT_EQ(p.As<int>(), 55);
    ASSERT_EQ(p.As < int const>(), 55);
    ASSERT_EQ(p.As<int &&>(), 55);

    p = 3.7f;

    ASSERT_EQ(p.As<float>(), 3.8f);
    ASSERT_EQ(p.As < float const>(), 3.7f);
    ASSERT_EQ(p.As<float &&>(), 3.7f);
}

TEST_F(App, UniversalParameter_ComplexTypes) {
    Gravity::Parameter p;

    p = std::vector<int>{1, 2, 3};

    ASSERT_EQ(p.As<std::vector<int>>(), (std::vector<int>{1, 2, 3}));
}
