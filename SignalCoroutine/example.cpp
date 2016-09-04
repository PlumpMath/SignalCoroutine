#include "SignalCoroutine.h"
#include <iostream>

boost::signals2::signal< void (void) > sigReadyRead;

boost::signals2::signal< void (const char *data, int len) >	sigReadData;

boost::signals2::signal< void ()> sigClose;

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
int main()
{
	boost::function< void (boost::shared_ptr<Context>) > coro = boost::bind( &readCoroutine, _1);
	spawn(coro);
	//example
	pushDataRead();

	CoroutineManage::instance().gc();
}