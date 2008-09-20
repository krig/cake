#ifndef __ARGLIST_H__
#define __ARGLIST_H__

#include <algorithm>

struct stringcmp
{
	stringcmp(const char* a) { this->a = a; }
	bool operator()(const char* b) const
	{
		return strcmp(a, b) == 0;
	}
	const char* a;
};

struct arglist
{
	typedef std::vector<const char*> vec_t;
	
	arglist()
	{
		args.reserve(16);
	}

	~arglist()
	{
		for (size_t i = 0; i < args.size(); ++i)
			if (args[i])
				free((void*)args[i]);
	}

	void add_arg(const char* str)
	{
		vec_t::iterator i;
		i = std::find_if(args.begin(),
						 args.end(),
						 stringcmp(str));
		if (i == args.end())
			args.push_back(strdup(str));
	}
	
	arglist& operator<<(const char* str)
	{
		add_arg(str);
		return *this;
	}
	
	// inserts list of space delimited arguments
	void insert_spacedelim(const char* str)
	{
		char tmp[1024];
		strncpy(tmp, str, 1024);
		char* tok = strtok(tmp, " \t\r\n");
		while (tok)
		{
			add_arg(tok);
			tok = strtok(0, " \t\r\n");
		}
	}

	char* const* get()
	{
		args.push_back(0);
		return (char* const*)&args[0];
	}

	const char* operator[](size_t index) const
	{
		return args[index];
	}
	
	
	size_t size() const { return args.size(); }
	
	void clear()
	{
		args.clear();
	}
	
	private:
	vec_t args;
};

#endif /* __ARGLIST_H__ */

