/**
    \file parameter.h
    \author Dmitry Kozlov
    \version 1.0
    \brief Header file containing generic parameter implementation for Gravity scene graph.

    Gravity generic parameter class is based on type erasure technique. This file contains implementation of
    helper classes and the parameter class itself.
 */
#pragma once

#include <iostream>
#include <stdexcept>
#include <typeindex>

namespace Gravity
{
	/**
        \brief Interface for type-erasure mechanism.

        Placeholder serves as a base class for a template implementation of different value holders.
	 */
	class Placeholder
	{
    public:
		virtual ~Placeholder() = default;

		/// Create a copy of an object.
		virtual Placeholder* Clone() = 0;

#ifdef ENABLE_TYPE_LOCK
		/// Return type index of an underlying value.
		/// Requires RTTI.
		virtual std::type_index GetTypeIndex() const = 0;
#endif
	};

    /**
        \brief Implementation of a Placeholder for a particular value type.

        Holder<T> keeps an information about underlying value type and implements Placeholder interface.
     */
	template <typename T> class Holder : public Placeholder
	{
	public:
		/// Construct from a value of type T (copy it)
		Holder(T const& val)
			: m_value(val)
		{
		}

		/// Construct from r-value of type T (move it)
		Holder(T&& val)
			: m_value(std::move(val))
		{
		}

		/// Create a copy of an object.
		Placeholder* Clone() override
		{
			return new Holder<T>(m_value);
		}

#ifdef ENABLE_TYPE_LOCK
        /// Return type index of an underlying value.
		/// Requires RTTI.
		std::type_index GetTypeIndex() const override
		{
			return std::type_index(typeid(T));
		}
#endif

		T m_value;
	};

    /**
        \brief The class which can hold the value of an arbitrary type.

        The parameter class holds a value of an arbitrary type. It allows type casts via As<T> method calls.
        Optionally the class supports type lock and type checking: If the parameter has been locked you can only
        assign to it the value of the type it currently has.
     */
	class Parameter
	{
	public:
        Parameter()
			: m_placeholder(nullptr)
#ifdef ENABLE_TYPE_LOCK
			, m_type_lock(false)
#endif
		{
		}

        Parameter(Parameter const& rhs)
			: m_placeholder(nullptr)
#ifdef ENABLE_TYPE_LOCK
			, m_type_lock(rhs.m_type_lock)
#endif
		{
			// Get the copy of a value
			m_placeholder = rhs.m_placeholder ? rhs.m_placeholder->Clone() : nullptr;
		}

		/// Construct from arbitrary value (only enabled for non-derived types, otherwise it masks copy ctor).
		template<typename T, typename = typename std::enable_if<!std::is_base_of<
			Parameter,
			typename std::decay<T>::type>::value>::type>
            Parameter(T&& val)
			: m_placeholder(new Holder<typename std::decay<T>::type>(std::forward<T>(val)))
#ifdef ENABLE_TYPE_LOCK
			, m_type_lock(false)
#endif
		{
		}

		/// Assign arbitrary value (only enabled for non-derived type, otherwise it masks an assignment operator).
		template <typename T, typename = typename std::enable_if<!std::is_base_of<
			Parameter,
			typename std::decay<T>::type>::value>::type> Parameter& operator = (T&& val)
		{
#ifdef ENABLE_TYPE_LOCK
			if (m_type_lock && m_placeholder && (m_placeholder->GetTypeIndex() != std::type_index(typeid(typename std::decay<T>::type))))
				throw std::bad_cast();
#endif
			Placeholder* holder = new Holder<typename std::decay<T>::type>(std::forward<T>(val));

			std::swap(m_placeholder, holder);

			delete holder;

			return *this;
		}

        Parameter& operator = (Parameter const& rhs)
		{
#ifdef ENABLE_TYPE_LOCK
			if ((m_type_lock && (m_placeholder && rhs.m_placeholder && (rhs.m_placeholder->GetTypeIndex() != m_placeholder->GetTypeIndex())))
				|| (m_placeholder && !rhs.m_placeholder))
				throw std::bad_cast();
#endif
			auto holder = rhs.m_placeholder ? rhs.m_placeholder->Clone() : nullptr;

			std::swap(m_placeholder, holder);

			delete holder;

			return *this;
		}

		~Parameter()
		{
			delete m_placeholder;
		}

		/// \brief Cast to a type T.
        /// \details If the type check is enabled attempt to cast to the type different from the one kept by the
        /// parameter results in std::bad_cast exception being thrown.
		template <typename T> typename std::decay<T>::type& As()
		{
			using MyType = typename std::decay<T>::type;

#ifdef ENABLE_TYPE_CHECK
			if (!m_placeholder || (m_placeholder && m_placeholder->GetTypeIndex() != std::type_index(typeid(MyType))))
				throw std::bad_cast();
#endif

			auto holder = static_cast<Holder<MyType>*>(m_placeholder);

			return holder->m_value;
		}

#ifdef ENABLE_TYPE_LOCK
		/// Lock the type of the parameter, meaning you can't assign value of a different type
		/// compared to the one currently kept.
		void SetTypeLock(bool type_lock)
		{
			m_type_lock = type_lock;
		}
#endif

	private:
        /// Value placeholder
		Placeholder* m_placeholder;

#ifdef ENABLE_TYPE_LOCK
        /// Optional type lock flag
		bool m_type_lock;
#endif
	};
}
