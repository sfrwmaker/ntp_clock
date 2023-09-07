/*
 * jsoncmd.h
 *
 *  Created on: 23 apr 2022
 *      Author: Alex
 */

#ifndef JSONRPC_H_
#define JSONRPC_H_

#include <stack>
#include "JsonParser.h"
#include "Arduino.h"
#include <FS.h>
#include <LittleFS.h>

typedef enum e_dst_type {
    TZ_NONE, TZ_SUMMER, TZ_WINTER
} t_dst;

//--------------------------------------------------- Configuration file parser -------------------------------
class SERIAL_PARSER: public JsonListener {
	public:
    	SERIAL_PARSER() : JsonListener()              		{ }
    	virtual			~SERIAL_PARSER(void)				{ }
    	virtual void 	key(String key)                		{ d_key = key; }
    	virtual void	endObject();
    	virtual void	startObject();
    	virtual void	startArray();
    	virtual void	endArray();
    	virtual void	startDocument();
    	virtual void	endDocument()                       { }
    	virtual void	whitespace(char c)					{ }
    	bool			parse(const char c);
    	void			init(void)							{ parser.setListener(this); }
	protected:
    	String				d_key;
    	std::stack<String>	s_array;
    	std::stack<String>	s_key;							// Json structure stack
        String              var_name;
        t_dst               tz_type     = TZ_NONE;
        uint8_t             dst_month   = 0;
        uint8_t             dst_day     = 0;
        uint8_t             dst_w_num   = 0;
        uint8_t             dst_w_day   = 0;
        uint16_t            dst_when    = 0;
        int16_t             dst_shift   = 0;
        uint16_t            br_min      = 0;
        uint16_t            br_max      = 0;
	private:
    	bool				is_body = false;
    	JsonStreamingParser parser;
};

class JSON_RPC : public SERIAL_PARSER {
	public:
		JSON_RPC()                                			{ }
		virtual			~JSON_RPC(void)						{ }
        void            readConfig(String& name);
		virtual void 	value(String value);
};

#endif
