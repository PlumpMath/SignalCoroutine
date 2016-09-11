#include "SignalCoroutine.h"
#include "SelectorContext.h"

#include <iostream>
boost::signals2::signal< void (void) > sigReadyRead;

boost::signals2::signal< void (const char *data, int len) >	sigReadData;

boost::signals2::signal< void ()> sigClose;

static MessageQueue<const char *> msgQueue;

void readCoroutine(boost::shared_ptr<Context> context)
{
	std::cout<<"begin test read data"<<std::endl;
	context->wait(sigReadyRead);

	while ( true )
	{
		boost::tuple< const char *,int > d = context->wait(sigReadData);
		if ( d.get<1>() == 0)
		{
			break;
		}
		std::cout<< d.get<0>() << std::endl;
	}
	context->wait(sigClose);
}

void pushDataRead()
{
	sigReadyRead();
	static const char *msg1 = "test data1";
	static const char *msg2 = "test data2";
	static const char *msg3 = "test data3";

	sigReadData(msg1, strlen(msg1));
	sigReadData(msg2, strlen(msg2));
	sigReadData(msg3, strlen(msg3));

	sigClose();
}

void consumer(boost::shared_ptr<Context> context)
{
	std::cout<<"begin consumer data"<<std::endl;

	while (true)
	{
		const char *msg = msgQueue.front(context);
		if ( msg == nullptr )
		{
			break;
		}
		else
		{
			std::cout<<msg<<std::endl;
		}
		msgQueue.pop();
	}
}

void producer()
{
	static const char *msg1 = "test data1";
	static const char *msg2 = "test data2";
	static const char *msg3 = "test data3";

	msgQueue.push(msg1);
	msgQueue.push(msg2);
	msgQueue.push(msg3);
	msgQueue.push(nullptr);
}

class referenceData:public boost::noncopyable
{
public:
	~referenceData()
	{

	}
};

class copyData
{
public:
	copyData()
	{
		copyed = false;
	}

	copyData(copyData &other)
	{
		if( other.copyed == false )
		{
			this->copyed = true;
			other.copyed = true;
		}
	}

	bool copyed;

protected:
	copyData &operator=(const copyData &other)
	{

	}
};

/************************************************************************/
/* 测试引用                                                                 */
/************************************************************************/
static boost::signals2::signal< void (referenceData &r) > sigReference;
void testReference(boost::shared_ptr<Context> context)
{
	referenceData &r = context->wait(sigReference);

}
void pushTestReference()
{
	referenceData d;
	sigReference(d);
}


static boost::signals2::signal< void (referenceData &r1, referenceData &r2) > sigReferenceV2;
void testReferenceV2(boost::shared_ptr<Context> context)
{
	boost::tuple<  referenceData&, referenceData & > d = context->wait(sigReferenceV2);

}
void pushTestReferenceV2()
{
	referenceData d1;
	referenceData d2;
	sigReferenceV2(d1, d2);	
}


static boost::signals2::signal< void (referenceData &r1, referenceData &r2, referenceData &r3) > sigReferenceV3;
void testReferenceV3(boost::shared_ptr<Context> context)
{
	boost::tuple< referenceData &, referenceData &, referenceData &> d = context->wait(sigReferenceV3);

}
void pushTestReferneceV3()
{
	referenceData d1;
	referenceData d2;
	referenceData d3;
	sigReferenceV3(d1, d2, d3);
}

/************************************************************************/
/* 测试值传递                                                            */
/************************************************************************/
static boost::signals2::signal< void (copyData) >	sigValue;
void testValue(boost::shared_ptr<Context> context)
{
	copyData d = context->wait(sigValue);
	//简单测试，不使用test框架
	assert(d.copyed);
}
void pushTestValue()
{
	copyData d;
	sigValue(d);
}

static boost::signals2::signal< void (copyData &, copyData &) > sigValueV2;
void testValueV2(boost::shared_ptr<Context> context)
{
	boost::tuple<copyData , copyData> d = context->wait(sigValueV2);
	copyData &r1 = d.get<0>();
	assert(d.get<0>().copyed);
	assert(d.get<1>().copyed);
}
void pushTestValueV2()
{
	copyData d1;
	copyData d2;
	sigValueV2(d1, d2);
}

static boost::signals2::signal< void (copyData &, copyData &, copyData &) > sigValueV3;
void testValueV3(boost::shared_ptr<Context> context)
{
	boost::tuple< copyData, copyData, copyData> d = context->wait(sigValueV3);
	assert(d.get<0>().copyed);
	assert(d.get<1>().copyed);
	assert(d.get<2>().copyed);
}
void pushTestValueV3()
{
	copyData d1;
	copyData d2;
	copyData d3;
	sigValueV3(d1, d2, d3);
}

static boost::signals2::signal< void (void)>	sigCaseVoid1;
int main()
{

	{
		//example read data
		boost::function< void (boost::shared_ptr<Context>) > coro = boost::bind( &readCoroutine, _1);
		spawn(coro);
		pushDataRead();
	}

	{
		//example message queue
		boost::function< void (boost::shared_ptr<Context>) > coro = boost::bind( &consumer, _1);
		spawn(coro);
		producer();
	}

	{
		//example reference
		boost::function< void (boost::shared_ptr<Context>) > coro = boost::bind( &testReference, _1);
		spawn(coro);
		pushTestReference();
	}

	{
		boost::function< void (boost::shared_ptr<Context>) > coro = boost::bind( &testReferenceV2, _1);
		spawn(coro);
		pushTestReferenceV2();
	}

	{
		boost::function< void (boost::shared_ptr<Context>) > coro = boost::bind( &testReferenceV3, _1);
		spawn(coro);
		pushTestReferneceV3();
	}

	//测试值传递，看一下是否会触发构造函数
	{
		boost::function< void (boost::shared_ptr<Context>) > coro = boost::bind( &testValue, _1);
		spawn(coro);
		pushTestValue();
	}

	//测试值传递，看一下是否会触发构造函数
	{
		boost::function< void (boost::shared_ptr<Context>) > coro = boost::bind( &testValueV2, _1);
		spawn(coro);
		pushTestValueV2();
	}

	//测试值传递，看一下是否会触发构造函数
	{
		boost::function< void (boost::shared_ptr<Context>) > coro = boost::bind( &testValueV3, _1);
		spawn(coro);
		pushTestValueV3();
	}

	CoroutineManage::instance().gc();
}