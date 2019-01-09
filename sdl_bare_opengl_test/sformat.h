#pragma once
struct IFormattable
{
	virtual int format(char *buffer, int buffer_size) = 0;
};

template<typename T>
struct CStupidFormattable : public IFormattable
{
	CStupidFormattable(T value)
		: value(value) {}
	T value;
	int format(char *buffer, int buffer_size) override
	{
		return sappend(this->value, buffer, buffer_size);
	}
};


struct String
{
	String(char *buffer, int length)
		: buffer(buffer), length(length) {}
	char *buffer;
	int length;
};


String sformat_core(const char *format, int num_formattables, IFormattable **formattables);


int sappend(int value, char * buffer, int buffer_size);
int sappend(double value, char * buffer, int buffer_size);
int sappend(float value, char * buffer, int buffer_size);
int sappend(unsigned int value, char * buffer, int buffer_size);
int sappend(long value, char * buffer, int buffer_size);
int sappend(unsigned long value, char * buffer, int buffer_size);
int sappend(long long value, char * buffer, int buffer_size);
int sappend(unsigned long long value, char * buffer, int buffer_size);
int sappend(char value, char * buffer, int buffer_size);
int sappend(const char * value, char * buffer, int buffer_size);


template<typename T0>
String sformat(const char *format, T0 arg0)
{
	CStupidFormattable<T0> formattable0 = CStupidFormattable<T0>(arg0);
	IFormattable *formattables[] = {
		&formattable0
	};

	return sformat_core(format, sizeof(formattables) / sizeof(formattables[0]), formattables);
}

template<typename T0, typename T1>
String sformat(const char *format, T0 arg0, T1 arg1)
{
	CStupidFormattable<T0> formattable0 = CStupidFormattable<T0>(arg0);
	CStupidFormattable<T1> formattable1 = CStupidFormattable<T1>(arg1);
	IFormattable *formattables[] = {
		&formattable0,
		&formattable1
	};

	return sformat_core(format, sizeof(formattables) / sizeof(formattables[0]), formattables);
}

template<typename T0, typename T1, typename T2>
String sformat(const char *format, T0 arg0, T1 arg1, T2 arg2)
{
	CStupidFormattable<T0> formattable0 = CStupidFormattable<T0>(arg0);
	CStupidFormattable<T1> formattable1 = CStupidFormattable<T1>(arg1);
	CStupidFormattable<T2> formattable2 = CStupidFormattable<T2>(arg2);
	IFormattable *formattables[] = {
		&formattable0,
		&formattable1,
		&formattable2
	};

	return sformat_core(format, sizeof(formattables) / sizeof(formattables[0]), formattables);
}

template<typename T0, typename T1, typename T2, typename T3>
String sformat(const char *format, T0 arg0, T1 arg1, T2 arg2, T3 arg3)
{
	CStupidFormattable<T0> formattable0 = CStupidFormattable<T0>(arg0);
	CStupidFormattable<T1> formattable1 = CStupidFormattable<T1>(arg1);
	CStupidFormattable<T2> formattable2 = CStupidFormattable<T2>(arg2);
	CStupidFormattable<T3> formattable3 = CStupidFormattable<T3>(arg3);
	IFormattable *formattables[] = {
		&formattable0,
		&formattable1,
		&formattable2,
		&formattable3
	};

	return sformat_core(format, sizeof(formattables) / sizeof(formattables[0]), formattables);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4>
String sformat(const char *format, T0 arg0, T1 arg1, T2 arg2, T3 arg3, T4 arg4)
{
	CStupidFormattable<T0> formattable0 = CStupidFormattable<T0>(arg0);
	CStupidFormattable<T1> formattable1 = CStupidFormattable<T1>(arg1);
	CStupidFormattable<T2> formattable2 = CStupidFormattable<T2>(arg2);
	CStupidFormattable<T3> formattable3 = CStupidFormattable<T3>(arg3);
	CStupidFormattable<T4> formattable4 = CStupidFormattable<T4>(arg4);
	IFormattable *formattables[] = {
		&formattable0,
		&formattable1,
		&formattable2,
		&formattable3,
		&formattable4
	};

	return sformat_core(format, sizeof(formattables) / sizeof(formattables[0]), formattables);
}

