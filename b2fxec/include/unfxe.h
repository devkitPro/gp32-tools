#ifndef _unfxe_h_included
#define _unfxe_h_included

class UnFXE {
private:
	void fix_key( long klen, unsigned char *key );
	unsigned char *decrypt_buffer( long klen, const unsigned char *key, 
					 long blen, unsigned char *buf, unsigned char* dst );
	unsigned char *parse_fxe( unsigned char *p, int fxelen );
	//

	unsigned char *m_fxedata;
	unsigned char *m_key;
	int m_keylen;
	int m_filelen;
	char m_icon[32*32];
	char m_gamename[32];
	char m_authorname[32];


public:
	UnFXE() : m_fxedata(NULL), m_key(NULL) {}
	~UnFXE() {
	}
	
	unsigned char *unfxe( unsigned char *data, int& datalen );
	const char* const getGameName() const { return m_gamename; }
	const char* const getAuthorName() const { return m_authorname; }
	const char* const getIcon() const { return m_icon; }
};




#endif
