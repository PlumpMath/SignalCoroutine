#include "SignalCoroutine.h"

void CoroutineManage::cleanup()
{
	this->Coroutines.clear();
}

void CoroutineManage::gc()
{
	auto iter = std::remove_if(
					this->Coroutines.begin(), 
					this->Coroutines.end(), 
					[&](ContextPtr &context)->bool
					{ 
						return context->Caller;
					});

	this->Coroutines.erase(iter, this->Coroutines.end());
}

CoroutineManage CoroutineManage::instance()
{
	//不考虑多线程
	static CoroutineManage in;
	return in;
}

//真正的启动协程
void run(boost::weak_ptr<Context> &context, Context::pull_coro_t &coro)
{
	boost::shared_ptr<Context> sp = context.lock();
	sp->_yield = &coro;
	sp->CallFunc(boost::ref(sp));
}

void spawn(boost::function<void (boost::shared_ptr<Context> c) > &f)
{
	boost::shared_ptr<Context> c(new Context());
	CoroutineManage::instance().Coroutines.push_back(c);
	c->CallFunc = f;
	//这里如果直接赋值shared_ptr,将会增加引用计数，导致互相引用
	c->CoroFunc = boost::bind(run, boost::weak_ptr<Context>(c), _1);
	c->Caller = Context::push_coro_t(c->CoroFunc);
	c->resume();
}