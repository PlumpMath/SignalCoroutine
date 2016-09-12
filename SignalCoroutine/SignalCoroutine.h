#pragma once
#include <boost/coroutine/asymmetric_coroutine.hpp>
#include <boost/function.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/tuple/tuple.hpp>
#include <queue>
#include <vector>
#include <boost/scope_exit.hpp>
#include <assert.h>
class Context;
class CoroutineClosure;


typedef boost::shared_ptr<Context> ContextPtr;
typedef boost::shared_ptr<CoroutineClosure>	CoroutineClosurePtr;

//上下文，主要保存协程所需要的数据
class Context:public boost::noncopyable
{
public:
	typedef boost::coroutines::asymmetric_coroutine< void *>::pull_type pull_coro_t;
	typedef boost::coroutines::asymmetric_coroutine< void *>::push_type push_coro_t;

	Context()
	{

	}

	~Context()
	{
	}

	//在高版本的VS里面,可以使用可变模板来代替
	void wait( boost::signals2::signal<void (void)> &s)
	{
		auto sig = s.connect( [&](void) 
		{
			this->resume();
		});
		pull_coro_t &yield = *_yield;
		yield();
		sig.disconnect();
	}

	void selectWait()
	{
		assert(this->CaseList.size() == 0);
		pull_coro_t &yield = *_yield;
		yield();
	}


	template <class T>
	T wait( boost::signals2::signal< void (T) > &s)
	{
		auto sig = s.connect([&](T v)
		{
			this->resume(v);
		});

		pull_coro_t &yield = *_yield;
		yield();
		sig.disconnect();
		void * result = yield.get();
		return *(T*)(result);
	}


	template <class T>
	T &wait( boost::signals2::signal< void (T &) > &s)
	{
		auto sig = s.connect([&](T &v)
		{
			this->resume(v);
		});

		pull_coro_t &yield = *_yield;
		yield();
		sig.disconnect();
		void *result = yield.get();
		return *(T*)(result);
	}


	template <class T0, class T1>
	boost::tuple<T0, T1> wait( boost::signals2::signal< void (T0, T1) > &s)
	{

		auto sig = s.connect([&](T0 v0, T1 v1)
		{
			this->resume(v0, v1);
		});

		pull_coro_t &yield = *_yield;
		yield();
		sig.disconnect();
		void *result = yield.get();

		boost::tuple<T0&, T1&> &t = *(boost::tuple<T0&, T1&>*)(result);
		return std::move(t);
	}


	template <class T0, class T1>
	boost::tuple<T0 &, T1&> wait( boost::signals2::signal< void (T0 &, T1 &)> &s)
	{
		auto sig = s.connect( [&](T0 &v1, T1 &v2)
		{
			this->resume(v1, v2);
		});

		pull_coro_t &yield = *_yield;
		yield();
		sig.disconnect();
		void *result = yield.get();
		boost::tuple<T0&, T1&> &t = *(boost::tuple<T0&, T1&>*)(result);
		return std::move(t);
	}


	template <class T0, class T1, class T2>
	boost::tuple<T0, T1, T2> wait( boost::signals2::signal< void (T0, T1, T2) > &s)
	{
		auto sig = s.connect([&](T0 v0, T1 v1, T2 V2)
		{
			this->resume(v0, v1, v2);
		});

		pull_coro_t &yield = *_yield;
		yield();
		void *result = yield.get();
		sig.disconnect();

		boost::tuple<T0, T1> &t = *(boost::tuple<T0, T1>*)(result);
		return std::move(t);
	}

	template <class T0, class T1, class T2>
	boost::tuple<T0&, T1&, T2&> wait( boost::signals2::signal< void (T0 &, T1 &, T2 &) > &s)
	{
		auto sig = s.connect( [&](T0 &v0, T1 &v1, T2 &v2) 
		{
			this->resume(v0, v1, v2);
		});

		pull_coro_t &yield = *_yield;
		yield();
		sig.disconnect();
		void *result = yield.get();

		boost::tuple<T0 &, T1 &, T2 &> &t  = *(boost::tuple<T0 &, T1 &, T2 &>*)(result);
		return std::move(t);
	}

	void resume(void)
	{
		Caller(nullptr);
	}

	template <class T>
	void resume(T &v)
	{
		Caller((void *)&v);
	}


	template <class T0, class T1>
	void resume(T0 &v0, T1 &v1)
	{
		boost::tuple<T0&, T1&> t(v0, v1);
		void *p = &t;
		Caller( (void *)&t);
	}

	template <class T0, class T1, class T2>
	void resume(T0 &v0, T1 &v1, T2 &v2)
	{
		boost::tuple< T0&, T1&, T2& > t(v0, v1, v2);
		Caller( (void *)&t);
	}

	push_coro_t	Caller;

	pull_coro_t *_yield;

	std::list<CoroutineClosurePtr>	CaseList;
protected:
	friend void run(boost::weak_ptr<Context> &context, pull_coro_t &coro);

	friend void spawn(boost::function<void (boost::shared_ptr<Context> c) > &f);

	boost::function<void (boost::shared_ptr<Context> c) > 	CallFunc;

	boost::function<void (pull_coro_t &)>	CoroFunc;		//一定要保存coroutine的func，因为push_coro_t里面是通过保存一个指向这个的引用

};

template <class T>
class MessageQueue
{
public:
	const T &front(boost::shared_ptr<Context> &context) const
	{
		if (_messages.size() > 0 )
		{
			return _messages.front();
		}
		context->wait(this->_sigResume, context);
		return _messages.front();
	}

	T &front(boost::shared_ptr<Context> &context)
	{
		if (_messages.size() > 0)
		{
			return _messages.front();
		}

		context->wait(this->_sigResume);

		return _messages.front();
	}

	void pop()
	{
		_messages.pop();
	}

	void push(const T &v)
	{
		_messages.push(v);
		this->_sigResume();
	}

protected:
	boost::signals2::signal< void (void) >	_sigResume;

	std::queue<T>	_messages;
};


//协程管理，主要是为了能够删除执行完毕的协程
class CoroutineManage:public boost::noncopyable
{
public:
	static CoroutineManage &instance();

	//清理已经执行完毕的协程
	//其实可以通过在resume完毕的时候执行回收资源，但是目前没有测试，所以先暂时手动回收资源
	void gc();

	void cleanup();

	std::vector< ContextPtr > Coroutines;
protected:
};

//生成一个协程
extern void spawn(boost::function<void (boost::shared_ptr<Context> c) > &f);
