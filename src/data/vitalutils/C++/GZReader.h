#pragma once
#define BUFLEN 16384
#include <string>
#include <vector>
#include <zlib.h>
using namespace std;

class GZWriter {
public:
	GZWriter(const char* path) {
		fi = gzopen(path, "wb");
		if (fi) opened_path = path;
	}

	virtual ~GZWriter() {
		if (fi) gzclose(fi);
	}

protected:
	gzFile fi;
	string opened_path;

public:
	bool write(const void* buf, unsigned int len) {
		return gzwrite(fi, buf, len) > 0;
	}
	bool write(const double& f) {
		return write(&f, sizeof(f));
	}
	bool write(const float& f) {
		return write(&f, sizeof(f));
	}
	bool write(unsigned char& b) {
		return write(&b, sizeof(b));
	}
	bool write(const string& s) {
		unsigned long len = s.size();
		if (gzwrite(fi, &len, sizeof(len)) <= 0) return false;
		return write(&s[0], len);
	}
	bool opened() const {
		return fi != 0;
	}
};

class GZReader {
public:
	GZReader(const char* path) {
		fi = gzopen(path, "rb");
		if (fi) opened_path = path;
	}

	virtual ~GZReader() {
		if(fi) gzclose(fi);
	}

protected:
	gzFile fi;
	string opened_path;
	unsigned long fi_remain = 0; // ������ �����Ƿ�
	unsigned char fi_buf[BUFLEN]; // ���� ���� ������
	const unsigned char* fi_ptr = fi_buf; // fi_buf���� ���� ���� ������
										  // ��Ȯ�� len byte�� ����. �� ������ true, 1����Ʈ�� �������� false�� ����
public:
	unsigned long read(void* dest, unsigned long len) {
		unsigned char* buf = (unsigned char*)dest;
		if (!buf) return 0;
		unsigned long nread = 0;
		while (len > 0) {
			if (len <= fi_remain) { // �����͸� �� �о ����ϸ�?
				memcpy(buf, fi_ptr, len); // �����ϰ� ����
				fi_remain -= len;
				fi_ptr += len;
				nread += len;
				break;
			} else if (fi_remain) { // �����ϸ�?
				memcpy(buf, fi_ptr, fi_remain); // �ϴ� �ִ°��� �� ����
				len -= fi_remain;
				buf += fi_remain;
				nread += fi_remain;
			}
			unsigned int unzippedBytes = gzread(fi, fi_buf, BUFLEN); // �߰��� �о����
			if (!unzippedBytes) return nread; // ���̻� ������ ������
			fi_remain = unzippedBytes;
			fi_ptr = fi_buf;
		}
		return nread;
	}

	bool skip(unsigned long len) {
		if (len <= fi_remain) {
			fi_remain -= len;
			fi_ptr += len;
			return true;
		} else if (fi_remain) {
			len -= fi_remain;
			fi_remain = 0;
		}
		return -1 != gzseek(fi, len, SEEK_CUR);
	}

	bool skip(unsigned long len, unsigned long& remain) {
		if (remain < len) return false;
		if (len <= fi_remain) {
			fi_remain -= len;
			fi_ptr += len;
			remain -= len;
			return true;
		} else if (fi_remain) {
			len -= fi_remain;
			remain -= fi_remain;
			fi_remain = 0;
		}
		
		z_off_t nskip = gzseek(fi, len, SEEK_CUR);
		if (-1 == nskip) return false;
		remain -= len;
		return true;
	}

	// x�� �а� remain�� ���ҽ�Ŵ
	bool fetch(long& x, unsigned long& remain) {
		if (remain < sizeof(x)) return false;
		unsigned long nread = read(&x, sizeof(x));
		remain -= nread;
		return (nread == sizeof(x));
	}

	bool fetch(unsigned long& x, unsigned long& remain) {
		if (remain < sizeof(x)) return false;
		unsigned long nread = read(&x, sizeof(x));
		remain -= nread;
		return (nread == sizeof(x));
	}

	bool fetch(float& x, unsigned long& remain) {
		if (remain < sizeof(x)) return false;
		unsigned long nread = read(&x, sizeof(x));
		remain -= nread;
		return (nread == sizeof(x));
	}

	bool fetch(double& x, unsigned long& remain) {
		if (remain < sizeof(x)) return false;
		unsigned long nread = read(&x, sizeof(x));
		remain -= nread;
		return (nread == sizeof(x));
	}

	bool fetch(short& x, unsigned long& remain) {
		if (remain < sizeof(x)) return false;
		unsigned long nread = read(&x, sizeof(x));
		remain -= nread;
		return (nread == sizeof(x));
	}

	bool fetch(unsigned short& x, unsigned long& remain) {
		if (remain < sizeof(x)) return false;
		unsigned long nread = read(&x, sizeof(x));
		remain -= nread;
		return (nread == sizeof(x));
	}

	bool fetch(char& x, unsigned long& remain) {
		if (remain < sizeof(x)) return false;
		unsigned long nread = read(&x, sizeof(x));
		remain -= nread;
		return (nread == sizeof(x));
	}

	bool fetch(unsigned char& x, unsigned long& remain) {
		if (remain < sizeof(x)) return false;
		unsigned long nread = read(&x, sizeof(x));
		remain -= nread;
		return (nread == sizeof(x));
	}

	bool fetch(string& x, unsigned long& remain) {
		unsigned long strlen; 
		if (!fetch(strlen, remain)) return false;
		if (strlen >= 1048576) {// > 1MB
			return false;
		}
		x.resize(strlen);
		unsigned long nread = read(&x[0], strlen);
		remain -= nread;
		return (nread == strlen);
	}

	bool opened() const {
		return fi != 0;
	}

	bool eof() const {
		return gzeof(fi) && !fi_remain;
	}

	void rewind() {
		gzrewind(fi);
		fi_remain = 0;
		fi_ptr = fi_buf;
	}
};

class BUF : public vector<unsigned char> {
	unsigned long pos = 0;
public:
	BUF(unsigned long len = 0) : vector<unsigned char>(len) { pos = 0; }
	void skip(unsigned long len) {
		pos += len;
	}
	void skip_str() {
		unsigned long strlen;
		if (!fetch(strlen)) return;
		pos += strlen;
	}
	bool fetch(unsigned char& x) {
		if (size() < pos + sizeof(x)) {
			pos = size();
			return false;
		}
		memcpy(&x, &(*this)[pos], sizeof(x)); // �� �ܿ��� ������ ���� ����
		pos += sizeof(x);
		return true;
	}
	bool fetch(unsigned short& x) {
		if (size() < pos + sizeof(x)) {
			pos = size();
			return false;
		}
		memcpy(&x, &(*this)[pos], sizeof(x)); // �� �ܿ��� ������ ���� ����
		pos += sizeof(x);
		return true;
	}
	bool fetch(unsigned long& x) {
		if (size() < pos + sizeof(x)) {
			pos = size();
			return false;
		}
		memcpy(&x, &(*this)[pos], sizeof(x)); // �� �ܿ��� ������ ���� ����
		pos += sizeof(x);
		return true;
	}
	bool fetch(float& x) {
		if (size() < pos + sizeof(x)) {
			pos = size();
			return false;
		}
		memcpy(&x, &(*this)[pos], sizeof(x)); // �� �ܿ��� ������ ���� ����
		pos += sizeof(x);
		return true;
	}
	bool fetch(double& x) {
		if (size() < pos + sizeof(x)) {
			pos = size();
			return false;
		}
		memcpy(&x, &(*this)[pos], sizeof(x)); // �� �ܿ��� ������ ���� ����
		pos += sizeof(x);
		return true;
	}
	bool fetch(string& x) {
		unsigned long strlen;
		if (!fetch(strlen)) return false;
		if (strlen >= 1048576) {// > 1MB
			return false;
		}
		x.resize(strlen);
		if (size() < pos + strlen) {
			pos = size();
			return false;
		}
		memcpy(&x[0], &(*this)[pos], strlen);
		pos += strlen;
		return true;
	}
};
