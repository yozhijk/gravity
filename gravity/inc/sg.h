/**
    \file sg.h
    \author Dmitry Kozlov
    \version 1.0
    \brief Header file containing generic scene graph implementation.

    SceneGraph represents a set of nodes of several kinds each of which can have parameters of an arbitrary type.
    Graph listeners are notified on graph structure / parameter values change and can react accordingly.
 */
#pragma once

#include <map>
#include <set>
#include <memory>
#include <iostream>
#include <list>
#include <functional>
#include <algorithm>

#include "parameter.h"

namespace Gravity
{
    /**
        \brief Scene graph class.

         The scene graph workflow is based on callbacks.
         There are three main APIs:
            * Creator API: defines which nodes can be created, what parameters they have, etc.
            * Client API: creates nodes, sets parameters, etc.
            * Observer API: registers listeners, reacts to events.
        The workflow:
            * Creator subclasses ParameterFactory to communicate what nodes have what parameters.
            * Creator creates scene graph object passing factory into it.
            * Client uses scene graph: creates nodes, manipulates them, etc.
            * Creator register observers to react on scene changes.
    */
    template<typename Key, typename NodeType, typename Parameter>
    class SceneGraph
    {
    public:

        /**
            \brief Scene graph node class.

            Scene graph node represents single scene graph entity and can have parameters of an arbitrary type.
        */
        class Node
        {
        public:
            /**
                \brief The node needs a link to its scene graph to fire event triggers and a param set.

                \param sg SceneGraph the node belongs to.
                \param type Node type.
                \param param_set The set of parameters for this node
             */
            Node(SceneGraph<Key, NodeType, Parameter> &sg, NodeType const &type, std::map<Key, Parameter> &&param_set)
                    : m_sg(sg), m_type(type), m_paramset(std::move(param_set))
            {
            }

            /// Return Node type.
            NodeType GetType() const
            { return m_type; }

            /// \brief Set parameter value.
            /// \details If a key does not exist std::runtime_error is thrown.
            /// \param key Parameter key
            /// \param value Parameter value
            template<typename T>
            void SetValue(Key const &key, T &&value)
            {
                // Try to find the parameter
                auto iter = m_paramset.find(key);

                if (iter == m_paramset.cend())
                    throw std::runtime_error("Requested parameter not found");

                // Forward the value
                iter->second = std::forward<T>(value);

                // Trigger scene graph notification
                m_sg.FireOnNodeParameterChange(this, key);
            }

            /// \brief Modify parameter value by passing a lambda modifier.
            /// \details Comes in handy for complex array-like parameters where manual event triggers are
            /// required. If a key does not exist std::runtime_error is thrown.
            /// \param key Parameter key.
            /// \param func A functor accepting value parameter.
            template<typename T, typename Func>
            void ModifyValue(Key const &key, Func &&func)
            {
                // Try to find the parameter
                auto iter = m_paramset.find(key);

                if (iter == m_paramset.cend())
                    throw std::runtime_error("Requested parameter not found");

                func(iter->second.template As<T>());

                // Trigger scene graph notification
                m_sg.FireOnNodeParameterChange(this, key);
            };

            /// \brief Get parameter value for a given key.
            /// \details If the key does not exist in this node std::runtime_error is thrown.
            template <typename T>
            typename std::decay<T>::type& GetValue(Key const& key)
            {
                // Try to find the parameter
                auto iter = m_paramset.find(key);

                if (iter == m_paramset.cend())
                    throw std::runtime_error("Requested parameter not found");

                return iter->second.template As<T>();
            }


        private:
            /// Scene graph
            SceneGraph<Key, NodeType, Parameter> &m_sg;
            /// Node type
            NodeType m_type;
            /// Parameter set
            std::map<Key, Parameter> m_paramset;
        };

        /**
            \brief Factory producing Node parameters based on Node types.

            This is the main component of creator API. The creator of the scene graph has to implement this to
            communicate to the graph which parameters to populate to which node.
         */
        class ParameterFactory
        {
        public:
            virtual ~ParameterFactory() = default;

            /// Produce the set of parameters for a given node type.
            virtual std::map<Key, Parameter> GetParameterSet(NodeType const &type) const = 0;
        };


        // Callback typedefs
        using OnNodeCreateCallback =
        std::function<void(Node *)>;
        using OnNodeDeleteCallback =
        std::function<void(Node *)>;
        using OnNodeParameterChangeCallback =
        std::function<void(Node *, Key)>;

        /**
            \brief Listener callback class

            FilteredCallback wraps callback function and adds a filter
            functionality to it. It checks the type of the node calling
            it and matches it versus m_filter internal set. It calls
            the function if either m_filter is empty (non-filtered) or
            node type is contained within m_filter
        */
        template<typename Func>
        struct FilteredCallback
        {
            /// Creates a filtered callback from a given function and the set of event types.
            FilteredCallback(Func func, std::set<NodeType> filter)
                    : m_func(func), m_filter(filter)
            {
            }

            /// Perfect-forward parameters to the callback function.
            template<typename... Args>
            void operator()(Node *node, Args &&... args)
            {
                // Call if the filter is empty
                if (m_filter.empty())
                {
                    m_func(node, std::forward<Args...>(args...));
                }
                else
                {
                    // Otherwise check the type
                    auto type = node->GetType();

                    // and call if the type is contained within m_filter
                    if (m_filter.find(type) != std::end(m_filter))
                    {
                        m_func(node, std::forward<Args...>(args...));
                    }
                }
            }

            /// Call applying a filter.
            void operator()(Node *node)
            {
                // Call if the filter is empty
                if (m_filter.empty())
                {
                    m_func(node);
                }
                else
                {
                    // Otherwise check the type
                    auto type = node->GetType();

                    // and call if the type is contained within m_filter
                    if (m_filter.find(type) != std::end(m_filter))
                    {
                        m_func(node);
                    }
                }
            }

            /// Callback
            Func m_func;
            /// Filter set
            std::set<NodeType> m_filter;
        };

        /// Create a scene graph with a given parameter factory.
        SceneGraph(ParameterFactory *param_factory) : m_param_factory(param_factory)
        { }

        ~SceneGraph() = default;

        SceneGraph(SceneGraph const &) = delete;

        SceneGraph &operator=(SceneGraph const &) = delete;

        /// Create a node of a specified type.
        Node *CreateNode(NodeType const &type)
        {
            // Allocate and construct the node
            auto node = new Node(*this, type, m_param_factory->GetParameterSet(type));
            // Emplace it into the scene
            m_nodes.emplace(node);
            // Notify the observers
            FireOnNodeCreate(node);
            // Return node pointer (clients use it as ID, no need to delete)
            return node;
        }

        /// Delete the node.
        void DeleteNode(Node *node)
        {
            // Try to find the node in our scene
            auto iter = std::find_if(m_nodes.cbegin(), m_nodes.cend(), [node](std::unique_ptr<Node> const &ptr)
            { return ptr.get() == node; });

            if (iter == m_nodes.cend())
                throw std::runtime_error("There is no such node to delete");

            // Notify observers
            FireOnNodeDelete(node);

            // Erase the node (automatically disposed by unique_ptr)
            m_nodes.erase(iter);
        }

        /// Register callback for a node creation.
        void RegisterOnNodeCreateCallback(OnNodeCreateCallback cb, std::set<NodeType> filter = {})
        {
            m_cb_create.emplace_back(cb, filter);
        }

        /// Register callback for a node deletion.
        void RegisterOnNodeDeleteCallback(OnNodeDeleteCallback cb, std::set<NodeType> filter = {})
        {
            m_cb_delete.emplace_back(cb, filter);
        }

        /// Register parameter change callback.
        void RegisterOnNodeParameterChangeCallback(OnNodeParameterChangeCallback cb, std::set<NodeType> filter = {})
        {
            m_cb_change.emplace_back(cb, filter);
        }


    private:
        /// Trigger OnNodeCreate callbacks.
        void FireOnNodeCreate(Node *node)
        {
            for (auto &cb: m_cb_create) cb(node);
        }

        /// Trigger OnNodeDelete callbacks.
        void FireOnNodeDelete(Node *node)
        {
            for (auto &cb: m_cb_delete) cb(node);
        }

        /// Trigger OnNodeParameterChange callbacks.
        void FireOnNodeParameterChange(Node *node, Key key)
        {
            for (auto &cb: m_cb_change) cb(node, key);
        }


    private:
        using NodeSet = std::set<std::unique_ptr<Node>>;
        /// Set of nodes for the scene.
        NodeSet m_nodes;
        // Parameter factory.
        std::unique_ptr<ParameterFactory> m_param_factory;
        // Callback containers.
        std::list<FilteredCallback<OnNodeCreateCallback>> m_cb_create;
        std::list<FilteredCallback<OnNodeDeleteCallback>> m_cb_delete;
        std::list<FilteredCallback<OnNodeParameterChangeCallback>> m_cb_change;
    };

    template<typename Key, typename NodeType, typename Parameter>
    std::ostream &operator<<(std::ostream &out, typename SceneGraph<Key, NodeType, Parameter>::Node const &node)
    {
        // TODO: implement me
        return out; //node.StreamOut(out);
    }


    template<typename Key, typename NodeType, typename Parameter>
    std::ostream &operator<<(std::ostream &out, SceneGraph<Key, NodeType, Parameter> const &sg)
    {
        // TODO: implement me
        return out; //sg.StreamOut(out);
    }

    using DefaultSceneGraph = SceneGraph<std::string, std::uint32_t, Parameter>;

    DefaultSceneGraph *CreateDefaultSceneGraph(DefaultSceneGraph::ParameterFactory *factory);
}
