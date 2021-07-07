#include <cstring>
#include "Core/Io/IStream.h"
#include "Core/Misc/TString.h"
#include "Core/System/PipeReader.h"

namespace traktor
{

T_IMPLEMENT_RTTI_CLASS(L"traktor.PipeReader", PipeReader, Object)

PipeReader::PipeReader(IStream* stream)
:	m_stream(stream)
{
}

PipeReader::~PipeReader()
{
}

PipeReader::Result PipeReader::readLine(std::wstring& outLine)
{
	char buffer[64];
	outLine.clear();

	// Pop line from line queue if any.
	if (!m_lines.empty())
	{
		outLine = m_lines.front();
		m_lines.pop_front();
		return RtOk;
	}

	// Cannot continue after stream has closed.
	if (!m_stream)
		return RtEnd;

	int64_t nrecv = m_stream->read(buffer, sizeof(buffer));
	if (nrecv < 0)
	{
		m_stream = nullptr;
		return RtEnd;
	}

	// Transform into lines.
	for (int64_t i = 0; i < nrecv; ++i)
	{
		char ch = buffer[i];

#if defined(__APPLE__) || defined(__LINUX__) || defined(__RPI__)
		if (ch == 10)
		{
			m_lines.push_back(mbstows(std::string(m_acc.begin(), m_acc.end())));
			m_acc.resize(0);
		}
		else
			m_acc.push_back(ch);
#else
		if (ch == 13)
		{
			m_lines.push_back(mbstows(std::string(m_acc.begin(), m_acc.end())));
			m_acc.resize(0);
		}
		else if (ch != 10)
			m_acc.push_back(ch);
#endif
	}

	// Pop line from queue.
	if (!m_lines.empty())
	{
		outLine = m_lines.front();
		m_lines.pop_front();
		return RtOk;
	}

	// No lines in queue.
	return RtEmpty;
}

}
