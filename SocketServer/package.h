#pragma once
#ifndef PACKAGE_H_
#define PACKAGE_H_

// instructions
#define TYPE_TIME 0x01
#define TYPE_NAME 0x02
#define TYPE_CLIENTLIST 0x03
#define TYPE_MESSAGE 0x04
#define TYPE_FEEDBACK 0x05
#define TYPE_FAIL 0x06

// 实际发包为 pkg_header + 对应的字符串
typedef struct pkg_header_
{
	char type;
	__int32 length;
	__int32 source;
	__int32 target;
} pkg_header;

#endif // !PACKAGE_H_