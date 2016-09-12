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
		if ( _conn.connected() )
		{
			_conn.disconnect();
		}
	}

protected:
	boost::signals2::connection _conn;
};

template <class T1>
class SelectCaseClosureV1:public CoroutineClosure, public boost::noncopyable
{
public:
	typedef boost::function<void (T1 p)> _callback_type;
	typedef boost::signals2::signal< void (T1 v) >	_signal_type;

	SelectCaseClosureV1(boost::shared_ptr< Context> &context, _signal_type &sig)
	{
		_conn = sig.connect( boost::bind(&SelectCaseClosureV1::doing, this, _1) );
		_context = context;
	}

	~SelectCaseClosureV1()
	{
	}

	_callback_type &Callback() { return _callback; }
protected:
	void doing(T1 v1)
	{
		auto context_ptr = _context.lock();
		context_ptr->CaseList.clear();
		_callback(v1);
		context_ptr->Caller(nullptr);
	}
	boost::weak_ptr< Context >	_context;

	_callback_type _callback;
};

template <class T1, class T2>
class SelectCaseClosureV2:public CoroutineClosure, public boost::noncopyable
{
public:
	typedef boost::function<void (T1 p1, T2 p2)> _callback_type;
	typedef boost::signals2::signal< void (T1 v1, T2 v2) >	_signal_type;

	SelectCaseClosureV2(boost::shared_ptr< Context> &context, _signal_type &sig)
	{
		_conn = sig.connect( boost::bind(&SelectCaseClosure::doing, this, _1, _2) );
		_context = context;
	}

	~SelectCaseClosureV2()
	{
	}

	_callback_type &Callback() { return _callback; }
protected:
	void doing(T1 v1, T2 v2)
	{
		auto context_ptr = _context.lock();
		context_ptr->CaseList.clear();
		_callback(v1, v2);
		context_ptr->Caller(nullptr);
	}
	boost::weak_ptr< Context >	_context;

	_callback_type _callback;
};

class SelectCaseClosure:public CoroutineClosure, public::boost::noncopyable
{
public:
	typedef boost::function< void (void) > _callback_type;
	typedef boost::signals2::signal< void (void) >	_signal_type;

	SelectCaseClosure(boost::shared_ptr< Context> &context, _signal_type &sig)
	{
		_conn = sig.connect( boost::bind(&SelectCaseClosure::doing, this) );
		_context = context;
	}

	~SelectCaseClosure()
	{
	}

	_callback_type &Callback() { return _callback; }
protected:
	void doing(void)
	{
		auto context_ptr = _context.lock();
		_callback();
		context_ptr->CaseList.clear();
		context_ptr->Caller(nullptr);
	}
	boost::weak_ptr< Context >	_context;

	_callback_type _callback;
};

class ContextAdaptr
{
public:
	template <class T1>
	static boost::function< void (T1)> &ContextCase(boost::shared_ptr<Context> &context, boost::signals2::signal< void (T1 ) > &sig)
	{
		boost::shared_ptr< SelectCaseClosureV1<T1> > Closure(new SelectCaseClosureV1<T1>(context, sig));
		context->CaseList.push_back(boost::static_pointer_cast<CoroutineClosure>(Closure));
		return Closure->Callback();
	}

	static boost::function< void (void)> &ContextCase(boost::shared_ptr<Context> &context, boost::signals2::signal< void (void) > &sig)
	{
		boost::shared_ptr< SelectCaseClosure > Closure(new SelectCaseClosure(context, sig));
		context->CaseList.push_back(boost::static_pointer_cast<CoroutineClosure>(Closure));
		return Closure->Callback();
	}
};


#define SelectBegin(_context)	{ auto &_tmp_context____ = _context;
#define SelectCase(_sig)	ContextAdaptr::ContextCase(_tmp_context____, _sig) = [&]
#define SelectEnd()	_tmp_context____->selectWait();}