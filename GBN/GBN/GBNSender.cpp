#include "stdafx.h"
#include "Global.h"
#include "GBNSender.h"


GBNSender::GBNSender():base(0),expectSequenceNumberSend(0),waitingState(false)
{
}


GBNSender::~GBNSender()
{
}



bool GBNSender::getWaitingState() {
	return waitingState;
}




bool GBNSender::send(const Message &message) {
	if (this->waitingState) { //发送方处于等待确认状态
		return false;
	}
	if (init_flag == 1) {
		for (int i = 0; i < Seqlenth; i++) {
			this->packetWaitingAck[i].seqnum = -1;
		}
		init_flag = 0;
	}
	if (this->expectSequenceNumberSend < base + N) {

		this->packetWaitingAck[expectSequenceNumberSend % Seqlenth].acknum = -1;								//忽略ack字段
		this->packetWaitingAck[expectSequenceNumberSend % Seqlenth].seqnum = this->expectSequenceNumberSend;     //初始化序列号
		this->packetWaitingAck[expectSequenceNumberSend % Seqlenth].checksum = 0;
		memcpy(this->packetWaitingAck[expectSequenceNumberSend % Seqlenth].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[expectSequenceNumberSend % Seqlenth].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[expectSequenceNumberSend % Seqlenth]);

		pUtils->printPacket("发送方发送报文", this->packetWaitingAck[expectSequenceNumberSend % Seqlenth]);
		if (base == expectSequenceNumberSend) {
			cout << "发送方启动计时器" << endl;
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);			//启动发送基序列方定时器
		}
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[expectSequenceNumberSend % Seqlenth]);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
		this->expectSequenceNumberSend++;
		cout << "此报文发送后，expectSequenceNumberSend为" << expectSequenceNumberSend << endl;
		if (this->expectSequenceNumberSend == base + N) {
			this->waitingState = true;//进入等待状态
		}
	}
	return true;
}

void GBNSender::receive(const Packet& ackPkt) {
	//	if (this->waitingState == true) {//如果发送方处于等待ack的状态，作如下处理；否则什么都不做
			//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//如果校验和正确，并且确认序号=发送方已发送并等待确认的数据包序号
	if (checkSum == ackPkt.checksum && ackPkt.acknum >= base) {
		//this->expectSequenceNumberSend = 1 - this->expectSequenceNumberSend;			
		int old_base = base;

		pUtils->printPacket("发送方正确收到确认", ackPkt);
		base = ackPkt.acknum + 1;
		for (int i = base +N; i < base + 8; i++) {
			packetWaitingAck[i % Seqlenth].seqnum = -1;					//初始化下一个窗口的序列号
		}
		cout << "发送方滑动窗口内容为 " << '[' << ' ';
		for (int i = base; i < base + N; i++) {
			if (packetWaitingAck[i % Seqlenth].seqnum == -1) {         //未发
				cout << '*' << ' ';
			}
			else {														//已发
				cout << packetWaitingAck[i % Seqlenth].seqnum << ' ';
			}
		}
		cout << ']' << endl;
		if (base == expectSequenceNumberSend)
		{
			cout << "已发送分组已全部接送，关闭计时器" << endl;
			this->waitingState = false;
			pns->stopTimer(SENDER, old_base);	//关闭定时器
		}
		else {
			pns->stopTimer(SENDER, old_base);//还没接收完，继续等待
			pns->startTimer(SENDER, Configuration::TIME_OUT, base);
			this->waitingState = false;
		}

	}
	else {
		if (checkSum != ackPkt.checksum) {
			cout << "发送方收到的ACK损坏" << endl;
		}
		else {
			cout << "发送方没有收到正确的序号，继续等待" << endl;
		}
	}
	//}	
}

void GBNSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	cout << "发送超时，回退N步" << endl;
	pns->stopTimer(SENDER, seqNum);
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);
	int i = base;
	do {
		cout << "重发" << i << "号报文" << endl;
		pUtils->printPacket("发送方定时器时间到，重发报文", this->packetWaitingAck[i % Seqlenth]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[i % Seqlenth]);			//重新发送数据包
		i++;
	} while (i != expectSequenceNumberSend);


}
