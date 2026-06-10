#include <map>
#include <string>

class MimeType
{

public:
	static std::map<std::string, std::string> mime_type;

private:
	MimeType();
	MimeType(const MimeType &other);
	MimeType &operator=(MimeType const &other);
	~MimeType();
};
