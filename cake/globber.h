#ifndef __GLOBBER_H__
#define __GLOBBER_H__

struct globber
{
	globber()
	{
		memset(&m_glob, 0, sizeof(glob_t));
	}

	globber(const char* pattern)
	{
		memset(&m_glob, 0, sizeof(glob_t));
		start(pattern);
	}
	
	~globber()
	{
		clear();
	}
	
	void start(const char* pattern)
	{
		glob(pattern, GLOB_TILDE|GLOB_NOSORT, 0, &m_glob);
	}
	
	void append(const char* pattern)
	{
		glob(pattern, GLOB_TILDE|GLOB_NOSORT|GLOB_APPEND, 0, &m_glob);
	}
	
	void clear()
	{
		if (m_glob.gl_pathc) {
			globfree(&m_glob);
			memset(&m_glob, 0, sizeof(glob_t));
		}	
	}
	
	int size() const
	{
		return m_glob.gl_pathc;
	}
	
	const char* operator[](size_t index) const
	{
		return (index < m_glob.gl_pathc)? m_glob.gl_pathv[index] : "";
	}
	
	private:
	glob_t m_glob;
};

#endif /* __GLOBBER_H__ */

