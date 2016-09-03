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
	//�����Ƕ��߳�
	static CoroutineManage in;
	return in;
}

//����������Э��
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
	//�������ֱ�Ӹ�ֵshared_ptr,�����������ü��������»�������
	c->CoroFunc = boost::bind(run, boost::weak_ptr<Context>(c), _1);
	c->Caller = Context::push_coro_t(c->CoroFunc);
	c->resume();
}