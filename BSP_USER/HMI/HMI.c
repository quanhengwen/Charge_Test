#include <HMI.h>
#include "stdio.h"
#include "string.h"
#include "sys.h"
#include "usart1.h"
#include "CSV_Database.h"
#include "rtthread.h"
#include "app.h"
#include "delay.h"
#include "rtc.h"
#include "debug.h"

/*
 * 批量检测条目限制变量 bit7:null bit6:null bit5:转换效率 bit4:OCP bit3:快充诱导 bit2:最大输出电流 bit1:额定电压 bit0:纹波峰值  1：为有效
 */
u8 HMI_TestLimit=0x3f;

/**
 * HMI响应处理函数
 * @param  HMI_Type HMI_Rx_String [页面类型指针 有效内容字符类型指针]
 * @return  HMI_Error  [description]
 */
HMI_Error USART_Solution(u8 HMI_Type,char* HMI_Rx_String)
{
	u16 i=0;
	
	//等待设备响应
	do{
		if(HMI_RX_FLAG) break;
		i++;
	}while(i<60000);
	//仍不能接收到标志位，返回无响应错误
	if(!HMI_RX_FLAG) return HMI_NoResponse;
	
	//判断当前返回的数据类型
	switch(USART_RX_BUF[0])
	{
		case HMI_Touch_Type :	HMI_Type = HMI_Touch_Type; break;
		case HMI_Page_Type :	HMI_Type = HMI_Page_Type; break;
		case HMI_Touchxy_Type :	HMI_Type = HMI_Touchxy_Type; break;
		case HMI_SleepThing_Type :	HMI_Type = HMI_SleepThing_Type; break;
		case HMI_String_Type :	HMI_Type = HMI_String_Type; break;
		case HMI_Vaule_Type :	HMI_Type = HMI_Vaule_Type; break;
		case HMI_Instr_OK :	HMI_Type = HMI_Instr_OK; break;
		default : break;
	}
		
	//从串口接收到的字符串提取有效数据
	for(i=1;i<=UASRT1_RX_BUFFER_LEN-4;i++)
	{
		HMI_Rx_String[i-1]=USART_RX_BUF[i];		
	}
	//加上结尾符号以方便识别
	HMI_Rx_String[i-1]='\0';
	//清除缓冲变量
	HMI_RX_FLAG=0;
	UASRT1_RX_BUFFER_LEN=0;
	
	return HMI_OK;
}

/**
 * HMI跳转页面指令
 * @param  Page_ID [要跳转的页面名称]
 * @return         [description]
 */
HMI_Error HMI_File_Page(u8 Page_ID)
{
	char str[30];
	char str2[10];
	
	my_itoa(Page_ID,str2);
	strcpy(str,"page ");
	strcat(str,str2);
	HMI_Print(str);
	
	return HMI_OK;
}

/**
	* 发送字符串至HMI对象
  * @param  str [要发送的字符串]
  * @return     [description]
  */
HMI_Error HMI_Print_Str(char* Object_ID,char* fmt)
{
	char str[60];
	
	strcpy(str,Object_ID);
	strcat(str,".txt=\"");
	strcat(str,fmt);
	strcat(str,"\"");
	/* 禁止调度以防止干扰串口接收 */
	rt_enter_critical();	//进入临界区
	HMI_Print(str);
	rt_exit_critical();		//退出临界区

	return HMI_OK;
}

/**
* 发送数组至HMI对象   !!!必须是无符号整数
 * @param  str [要发送的数值]
 * @return     [description]
 */
HMI_Error HMI_Print_Val(char* Object_ID,u16 varible)
{
	char str[60];
	char temp_str[10];
	
	strcpy(str,Object_ID);
	strcat(str,".val=");
	my_itoa(varible,temp_str);
	strcat(str,temp_str);
	HMI_Print(str);

	return HMI_OK;
}

/**
 * HMI发送
 * @param  str [description]
 * @return     [description]
 */
HMI_Error HMI_Print(char* str)
{
	printf("%s\n", str);
	HMI_Send_TXEND();
	
	return HMI_OK;
}

/**
 * HMI当前页面确认
 * @param  Page_ID [页面ID]
 * @return     [description]
 */
HMI_Error HMI_Page_ACK(u8 Page_ID)
{
	u8 Type;
	char str[10];
	HMI_Print("sendme");
	if(!USART_Solution(Type,str)) return HMI_NoResponse;
	if(Type!=HMI_Page_Type) return HMI_Parse_Error;
	if(str[0]!=Page_ID) return HMI_Page_Error;
	
	return HMI_OK;
}

/*
 * 在界面显示当前测试标准参数
 */
/**
 * 在标准选择界面中显示当前参数
 * @param None   
 * @return HMI_Error
 * @brief 
 **/
HMI_Error HMI_StandardPage_Show(TestParameters_Type *Parameters)
{
	char Num_str[10];
	u8 Code_temp;
	
	/* 显示额定输出电压 */
	my_itoa(Parameters->Vout,Num_str);
	HMI_Print_Str("t0",Num_str);
	Num_str[0]='\0';
	/* 显示输出电压容差 */
	my_itoa(Parameters->Vout_Tolerance,Num_str);
	HMI_Print_Str("t1",Num_str);
	Num_str[0]='\0';
	/* 显示最大输出电流值 */
	my_itoa(Parameters->Cout_Max,Num_str);
	HMI_Print_Str("t2",Num_str);
	Num_str[0]='\0';
	/* 显示纹波电压 */
	my_itoa(Parameters->V_Ripple,Num_str);
	HMI_Print_Str("t3",Num_str);
	Num_str[0]='\0';
	/* 显示转换效率 */
	my_itoa(Parameters->Efficiency,Num_str);
	HMI_Print_Str("t4",Num_str);
	Num_str[0]='\0';
	/* 显示过流保护开关 */
	if(Parameters->Safety_Code)	Code_temp=1;
	else Code_temp=0;
	HMI_Print_Val("bt1",Code_temp);
	/* 显示快充诱导开关 */
	HMI_Print_Val("FastCharg_STA",Parameters->Quick_Charge);

	return HMI_OK;
}

/**
* 获取HMI控件的值  获取的值都为字符类型，如获取为数值需atoi函数
 * @param  Page_ID [页面ID]
 * @return     [description]
 */
HMI_Error HMI_Get(u8 Object_Type,char* Object_ID,char* fmt)
{
	char str[20];
	char str2[10];
	u8 i=0,n=0;
	u8 res=HMI_OK;
	
	//要获取的内容为字符串类型
	if(Object_Type==HMI_String_Type)
	{
		strcpy(str,"get ");
		strcat(str,Object_ID);
		strcat(str,".txt");
		HMI_Print(str);
		delay_ms(100); 	//延时一段时间待串口接收完毕
		//等待HMI响应
		res=USART_Solution(HMI_String_Type,fmt);
	}
	//要获取的内容为数值类型
	else if(Object_Type==HMI_Vaule_Type)
	{
		strcpy(str,"get ");
		strcat(str,Object_ID);
		strcat(str,".val");
		HMI_Print(str);
		delay_ms(100); 	//延时一段时间待串口接收完毕
		//等待HMI响应
		res=USART_Solution(HMI_Vaule_Type,str2);
		while(str2[i]!='\0')
		{
			if(str2[i]>=0&&str2[i]<=9) n=10*n+str2[i];
			i++;
		}
		my_itoa(n,fmt);			
	}
	
	return res;
}
/**
 * 将标准设置界面的参数文本写进全局标准结构体
 * @param   TestParameters_Structure[全局变量,在app.c定义]
 * @return     [description]
 */
HMI_Error HMI_Standard_Atoi(void)
{
	char str[20];
	u8 bit_temp;	//位缓存
	HMI_Error res=HMI_OK;
	
	/* 获取额定输出电压 */
	sprintf(str,"");
	res=HMI_Get(HMI_String_Type,"t0",str);
	TestParameters_Structure[Standard_val].Vout=my_atoi(str);
	/* 获取电压容差 */
	sprintf(str,"");
	res=HMI_Get(HMI_String_Type,"t1",str);
	TestParameters_Structure[Standard_val].Vout_Tolerance=my_atoi(str);
	/* 获取最大输出电流 */
	sprintf(str,"");
	res=HMI_Get(HMI_String_Type,"t2",str);
	TestParameters_Structure[Standard_val].Cout_Max=my_atoi(str);
	/* 获取纹波电压 */
	sprintf(str,"");
	res=HMI_Get(HMI_String_Type,"t3",str);
	TestParameters_Structure[Standard_val].V_Ripple=my_atoi(str);
	/* 获取转换效率 */
	sprintf(str,"");
	res=HMI_Get(HMI_String_Type,"t4",str);
	TestParameters_Structure[Standard_val].Efficiency=my_atoi(str);
	/* 获取过流保护开关 */
	sprintf(str,"");
	res=HMI_Get(HMI_Vaule_Type,"bt1",str);
	bit_temp=my_atoi(str);
	TestParameters_Structure[Standard_val].Safety_Code=bit_temp;
	/* 获取快充诱导开关 */
	strcpy(str,"");
	res=HMI_Get(HMI_Vaule_Type,"FastCharg_STA",str);
	TestParameters_Structure[Standard_val].Quick_Charge=my_atoi(str);
	
	return res;
}

/**
 * 将测试门限设置呈现给测试门限设置界面  批量检测条目限制变量 bit7:转换效率 bit6:上电时间 bit5:电路保护 bit4:OCP bit3:OVP bit2:cmax bit1:vmax bit0:纹波  1：为有效
 * @param   []
 * @return     [description]
 */
HMI_Error HMI_TestLimit_Itoa(void)
{
	u8 temp=0;
	u8 temp2=0;
	u8 res=HMI_OK;
	
	temp2=HMI_TestLimit;
	
	if(temp2&0x01) temp=1; else temp=0;
	HMI_Print_Val("bt0",temp);
	if(temp2&0x02) temp=1; else temp=0;
	HMI_Print_Val("bt1",temp);
	if(temp2&0x04) temp=1; else temp=0;
	HMI_Print_Val("bt2",temp);
	if(temp2&0x08) temp=1; else temp=0;
	HMI_Print_Val("bt3",temp);
	if(temp2&0x10) temp=1; else temp=0;
	HMI_Print_Val("bt4",temp);
	if(temp2&0x20) temp=1; else temp=0;
	HMI_Print_Val("bt5",temp);
	if(temp2&0x40) temp=1; else temp=0;
	HMI_Print_Val("bt6",temp);
	if(temp2&0x80) temp=1; else temp=0;
	HMI_Print_Val("bt7",temp);
	
	return res;
}

/**
 * 测试门限设置界面数值赋予全局测试门限变量
 * @param   []
 * @return     [description]
 */
HMI_Error HMI_TestLimit_Atoi(u8 * LimitVal)
{
	char str[5];
	u8 temp1=0;
	u8 temp2=0;
	HMI_Error res=HMI_OK;
	
	res=HMI_Get(HMI_Vaule_Type,"bt0",str);
	temp1=my_atoi(str);
	temp2=temp1;
	res=HMI_Get(HMI_Vaule_Type,"bt1",str);
	temp1=my_atoi(str);
	temp1=temp1<<1;
	temp2=temp2|temp1;
	res=HMI_Get(HMI_Vaule_Type,"bt2",str);
	temp1=my_atoi(str);
	temp1=temp1<<2;
	temp2=temp2|temp1;
	res=HMI_Get(HMI_Vaule_Type,"bt3",str);
	temp1=my_atoi(str);
	temp1=temp1<<3;
	temp2=temp2|temp1;
	res=HMI_Get(HMI_Vaule_Type,"bt4",str);
	temp1=my_atoi(str);
	temp1=temp1<<4;
	temp2=temp2|temp1;
	res=HMI_Get(HMI_Vaule_Type,"bt5",str);
	temp1=my_atoi(str);
	temp1=temp1<<5;
	temp2=temp2|temp1;
	res=HMI_Get(HMI_Vaule_Type,"bt6",str);
	temp1=my_atoi(str);
	temp1=temp1<<6;
	temp2=temp2|temp1;
	res=HMI_Get(HMI_Vaule_Type,"bt7",str);
	temp1=my_atoi(str);
	temp1=temp1<<7;
	temp2=temp2|temp1;
	
	
	*LimitVal = temp2;   //将参数传给全局变量  
	
	return res;
}

/**
 * 将时间参数呈现给HMI界面
 * @param   []
 * @return     [description]
 */
HMI_Error HMI_RTC_Show(void)
{
	char str1[20];

	RTC_TimeTypeDef RTC_TimeStruct;
	RTC_DateTypeDef RTC_DateStruct;
	
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct); //获取当前日期
	RTC_GetTime(RTC_Format_BIN,&RTC_TimeStruct);  //获取当前时间
	
	sprintf(str1,"%d-%d-%d %d:%d",RTC_DateStruct.RTC_Year,RTC_DateStruct.RTC_Month,
	RTC_DateStruct.RTC_Date,RTC_TimeStruct.RTC_Hours,RTC_TimeStruct.RTC_Minutes);
	
	HMI_Print_Str("t1",str1);
	
	return HMI_OK;
}

HMI_Error HMI_RTC_Atoi(void)
{
	char str1[20];
	char str2[10];
	u8 year,month,day,hour,min;
	int res1;
	u8 res2;
	/* 禁止调度以防止干扰串口接收 */
	rt_enter_critical();	//进入临界区
	if(HMI_Get(HMI_String_Type,"t2",str1)) return 1;
	rt_exit_critical();		//退出临界区
	str2[0]=str1[0];str2[1]=str1[1];str2[2]='\0';
	year=my_atoi(str2);
	str2[0]=str1[2];str2[1]=str1[3];str2[2]='\0';
	month=my_atoi(str2);
	str2[0]=str1[4];str2[1]=str1[5];str2[2]='\0';
	day=my_atoi(str2);
	str2[0]=str1[6];str2[1]=str1[7];str2[2]='\0';
	hour=my_atoi(str2);
	str2[0]=str1[8];str2[1]=str1[9];str2[2]='\0';
	min=my_atoi(str2);
	logging_debug("%d-%d-%d %d:%d",year,month,day,hour,min);
	if(month>12||month<1||day>31||day<1||day>31||hour>24||hour<1||min>60) return HMI_Parse_Error;
	RTC_Set_Date(year,month,day,1);
	res1=hour-12;
	if(res1<=0) res2=RTC_H12_AM;
	else res2=RTC_H12_PM;
	RTC_Set_Time(hour,min,0,res2);
	
	return HMI_OK;
}

HMI_Error HMI_ShowBatch(void)
{
	char str[5][10];
	char Event_Info[3];
	u8 Exit_Flag=0;
	while(1)
	{
		HMI_Print_Str("t0",str[0]);
		HMI_Print_Str("t1",str[1]);
		HMI_Print_Str("t2",str[2]);
		HMI_Print_Str("t3",str[3]);
		HMI_Print_Str("t4",str[4]);
		while(USART_Solution(HMI_Vaule_Type,Event_Info));
		switch(Event_Info[0])
		{
			case 0x01: break;//进入第一条目
			case 0x02: break;//进入第二条目
			case 0x03: break;//进入第三条目
			case 0x04: break;//进入第四条目
			case 0x05: break;//进入第五条目
			case 0x06: break;//上翻
			case 0x07: break;//下翻
			case 0x08: Exit_Flag=1;break;//退出
		}
		if(Exit_Flag) break;
	}
	HMI_File_Page(1);
	
	return HMI_OK;
}

/*
 * 获取批量名称并创建新批量目录
 */
HMI_Error HMI_Creat_NewBatch(void)
{
	char str[20];
	HMI_Error res;
	
	/* 禁止调度以防止干扰串口接收 */
	rt_enter_critical();	//进入临界区
	if(HMI_Get(HMI_String_Type,"t1",str)) return 1;
	rt_kprintf("name=%s",str);
	res=Creat_NewBatchDir(str);
	rt_kprintf("code=%d\r\n",res);
	rt_exit_critical();		//退出临界区	

	
	return 0;
}

/*
 * 系统运行错误执行(使程序停止运行)
 */
void HMI_ShowError(u16 Running_Code)
{
	char str[10];
	
	
	HMI_File_Page(6);	//跳转到错误显示界面
	my_itoa(Running_Code,str);
	HMI_Print_Str("t1",str);
	
	while(1);
}

