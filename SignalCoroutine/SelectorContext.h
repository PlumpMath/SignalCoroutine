#pragma once

#include "SignalCoroutine.h"

#define selector(_context)	
#define on(_sig)	_sig.connect(
#define then(...)		[&](__VA_ARGS__)
#define then_end	)

class CoroutineClosure
{
public:
	virtual ~CoroutineClosure() = 0
	{

	}
};

class ContextClosure:public CoroutineClosure, public boost::noncopyable
{
public:
	ContextClosure(boost::shared_ptr< Context> &context, boost::signals2::signal< void (void) > &sig)
	{
		_conn = sig.connect( boost::bind(&ContextClosure::doing, this) );
	}

	void operator=(boost::function< void (void) > &lambda)
	{
		_callback = lambda;
	}

protected:
	void doing(void)
	{
		auto context_ptr = _context.lock();
		context_ptr->_yield->get();
		_callback();
		_conn.disconnect();
		context_ptr->Caller(nullptr);
	}
	boost::weak_ptr< Context >	_context;

	boost::function< void (void) > _callback;

	boost::signals2::connection _conn;
};

ContextClosure &ContextCase(boost::shared_ptr<Context> &context, boost::signals2::signal< void (void) > &sig)
{
}