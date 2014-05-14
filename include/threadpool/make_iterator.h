/** @file threadpool/make_iterator.h
 *
 * Threadpool for C++11, make iterator from function
 *
 * @copyright	2014 Ruediger Helsch, Ruediger.Helsch@t-online.de
 * @license	All rights reserved. Use however you want. No warranty at all.
 * $Revision: 1.3 $
 * $Date: 2014/05/14 14:06:41 $
 */
#ifndef THREADPOOL_MAKE_ITERATOR_H
#define THREADPOOL_MAKE_ITERATOR_H

#include <iterator>
#include <type_traits>
#include <stdexcept> // for std::out_of_range
#include <memory> // for std::shared_ptr





namespace ThreadPoolImpl {

    namespace impl {



	/**
	 * An assignable value-type for reference types
	 *
	 * @tparam T
	 *		The type to transport
	 *
	 * The ref_value is default-constructable, constructable and
	 * assignable from T references, copy- and move- constructable
	 * and -assignable. To get the value out of it, call get() for
	 * a const reference or move() for an rvalue reference. For
	 * lvalue references, move() falls back to an lvalue
	 * reference.
	 */
	template<class T, class Enable = void>
	class ref_value;

	template<class T>
	class ref_value<T, typename std::enable_if<!std::is_reference<T>::value>::type> {
	    typename std::remove_cv<T>::type value;
	public:
	    ref_value() { }
	    ref_value(const T& x) : value(x) { }
	    ref_value(T&& x) : value(std::forward<T>(x)) { }
	    ref_value(const ref_value&) = default;
	    ref_value(ref_value&&) = default;
	    ref_value& operator=(const T& x) { value = x; return *this; }
	    ref_value& operator=(T&& x) { value = std::forward<T>(x); return *this; }
	    ref_value& operator=(const ref_value&) = default;
	    ref_value& operator=(ref_value&&) = default;
	    const typename std::remove_volatile<T>::type& get() const { return value; }
	    typename std::remove_cv<T>::type&& move() { return std::move(value); }
	};
	 
	template<class T>
	class ref_value<T, typename std::enable_if<std::is_lvalue_reference<T>::value>::type> {
	    typename std::remove_reference<T>::type* value;
	public:
	    ref_value() { }
	    ref_value(T x) : value(&x) { }
	    ref_value(const ref_value&) = default;
	    ref_value(ref_value&&) = default;
	    ref_value& operator=(T x) { value = &x; return *this; }
	    ref_value& operator=(const ref_value&) = default;
	    ref_value& operator=(ref_value&&) = default;
	    T get() const { return *value; }
	    T move() { return *value; }
	};

	template<class T>
	class ref_value<T, typename std::enable_if<std::is_rvalue_reference<T>::value>::type> {
	    typename std::remove_reference<T>::type* value;
	public:
	    ref_value() { }
	    ref_value(T x) : value(&x) { }
	    ref_value(const ref_value&) = default;
	    ref_value(ref_value&&) = default;
	    ref_value& operator=(T x) { value = &x; return *this; }
	    ref_value& operator=(const ref_value&) = default;
	    ref_value& operator=(ref_value&&) = default;
	    const T& get() const { return std::move(*value); }
	    T move() { return std::move(*value); }
	};



	/**
	 * An input iterator made from a function
	 *
	 * The function is called with no arguments and must return
	 * the next value of the input sequence or throw an
	 * std::out_of_range exception if the input sequence has
	 * ended. After throwing the exception once, the function is
	 * no more called.
	 *
	 * Implements only two member functions doing something. The
	 * operator==() returns true if both iterators are at the end
	 * or both iterators are not at the end. The operator*()
	 * returns the next value by rvalue reference.
	 */
	template<class Function>
	class FunctionInputIterator
	    : public std::iterator<std::input_iterator_tag,
				   decltype(std::declval<Function>()())> {
	    typedef std::iterator<std::input_iterator_tag,
				  decltype(std::declval<Function>()())> Base;

	    /*
	      Must share the values between copied interators, because
	      shared values may be compared, and during comparision a
	      value may need to be read, which should be available
	      through the original value of the iterator.
	    */
	    struct Values {
		bool last = false;
		bool value_valid = false;
		Function fun;
		ref_value<typename Base::value_type> value;
		Values(const Function& fun) : fun(fun) { }
		Values(Function&& fun) : fun(std::forward<Function>(fun)) { }
		Values(const Values& x) = default;
	    };

	    std::shared_ptr<Values> v;

	public:

	    FunctionInputIterator() = default;
	    FunctionInputIterator(const Function& fun) : v(new Values(fun)) { }
	    FunctionInputIterator(Function&& fun, bool last = false) : v(new Values(std::forward<Function>(fun))) { }
	    FunctionInputIterator(const FunctionInputIterator&) = default;
	    FunctionInputIterator(FunctionInputIterator&&) = default;
	    FunctionInputIterator& operator=(const FunctionInputIterator& x) = default;
	    FunctionInputIterator& operator=(FunctionInputIterator&&) = default;
	    FunctionInputIterator& operator++() { return *this; }
	    FunctionInputIterator& operator++(int) { return *this; }

	    typename Base::value_type&& operator*() const
	    {
		// No need to check for empty v, caller must have checked.
		if (v->value_valid) {
		    v->value_valid = false;
		} else if (!v->last) {
		    try {
			v->value = v->fun();
		    } catch (std::out_of_range) {
			v->last = true;
		    }
		}

		if (v->last)
		    throw std::out_of_range("");

		return v->value.move();
	    }

	    bool operator==(const FunctionInputIterator& x) const {

		if (v != nullptr && !(v->last || v->value_valid)) {
		    try {
			v->value = v->fun();
			v->value_valid = true;
		    } catch (std::out_of_range) {
			v->last = true;
		    }
		}

		if (x.v != nullptr && !(x.v->last || x.v->value_valid))
		    return x.operator==(*this);

		return (v == nullptr || v->last) == (x.v == nullptr || x.v->last);
	    }

	    bool operator!=(const FunctionInputIterator& x) const {
		return !operator==(x);
	    }

	};



	/*
	 * An input range made from a function
	 *
	 * The function is called with no arguments and must return
	 * the next value of the input sequence or throw an
	 * std::out_of_range exception if the input sequence has
	 * ended. After throwing the exception once, the function is
	 * no more called.
	 *
	 * Input iterators are frequently used as pairs delimiting a
	 * range. Use member functions begin() and end() of the range
	 * as `first` and `last` iterators for algorithms on iterator
	 * ranges, or use the range itself as input for algorithms
	 * working on containers or as the range argument in a
	 * range-based for statement.
	 */
	template<class Function>
	class FunctionInputIteratorRange {

	    std::shared_ptr<Function> fun;

	public:

	    FunctionInputIteratorRange(const Function& fun) : fun(new Function(fun)) { }
	    FunctionInputIteratorRange(Function&& fun) : fun(new Function(std::forward<Function>(fun))) { }
	    FunctionInputIteratorRange(const FunctionInputIteratorRange&) = default;
	    FunctionInputIteratorRange(FunctionInputIteratorRange&&) = default;
	    FunctionInputIteratorRange& operator=(const FunctionInputIteratorRange&) = default;
	    FunctionInputIteratorRange& operator=(FunctionInputIteratorRange&&) = default;
	    FunctionInputIterator<Function> begin() { return FunctionInputIterator<Function>(*fun.get()); }
	    FunctionInputIterator<Function> end() { return FunctionInputIterator<Function>(); }
	};



	/**
	 * An output iterator made from a function
	 *
	 * The function is called with one argument, which is whatever
	 * is assigned to the value_type of the iterator.
	 *
	 * Implements only one member function doing something: The
	 * operator=() calls the function with its argument.
	 */
	template<class Function>
	class FunctionOutputIterator
	    : public std::iterator<std::output_iterator_tag, void, void, void, void> {

	    std::shared_ptr<Function> fun;

	public:
	    FunctionOutputIterator() { }
	    FunctionOutputIterator(const Function& fun) : fun(new Function(fun)) { }
	    FunctionOutputIterator(Function&& fun) : fun(new Function(std::move(fun))) { }
	    FunctionOutputIterator(const FunctionOutputIterator&) = default;
	    FunctionOutputIterator(FunctionOutputIterator&&) = default;
	    FunctionOutputIterator& operator=(const FunctionOutputIterator&) = default;
	    FunctionOutputIterator& operator=(FunctionOutputIterator&&) = default;
	    FunctionOutputIterator& operator++() { return *this; }
	    FunctionOutputIterator& operator++(int) { return *this; }
	    FunctionOutputIterator& operator*() { return *this; }
	    template<class Arg> void operator=(Arg&& arg) { (*fun.get())(std::forward<Arg>(arg)); }
	};



    } // End of namespace impl

} // End of namespace ThreadPoolImpl





namespace threadpool {

    /**
     * Create an input range from a function
     */
    template<class Function>
    ThreadPoolImpl::impl::FunctionInputIteratorRange<Function>
    make_function_input_range(Function&& fun) {
	return ThreadPoolImpl::impl::FunctionInputIteratorRange<Function>(std::forward<Function>(fun));
    }

    /**
     * Create an output iterator from a function
     */
    template<class Function>
    ThreadPoolImpl::impl::FunctionOutputIterator<Function>
    make_function_output_iterator(Function&& fun) {
	return ThreadPoolImpl::impl::FunctionOutputIterator<Function>(std::forward<Function>(fun));
    }
 
} // End of namespace threadpool





#endif // !THREADPOOL_MAKE_ITERATOR_H
