#pragma once
#include <iostream>
#include <string>
#include <exception>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "boost/locale.hpp"
#include <regex>
#include <codecvt>
#include <vector>
#include <algorithm>
#include <pqxx/pqxx>

class Server {
private:
	std::wstring zagolovok_otvet = L"HTTP/1.1 200 OK\r\n\Content-Type: text/html\r\n\r\n";
	std::wstring stranica_prefix =
		L"<!DOCTYPE html>"
		"<html lang ='ru' class=''>"
		"<head>"
		"<meta charset=\"UTF-8\">"
		"<title>ПОИСКОВИК</title>"
		"<style>"
		".box {margin-top: 100px;}"
		"input {width: 500px; height: 30px;}"
		"button {height: 30px;}"
		"</style>"
		"</head>"
		"<body>"
		"<form  class =\"box\"  role =\"search\" id =\"form\" action=\"\" method=\"post\">"
		"<input type =\"search\" id =\"query\" name=\"zapros\" placeholder=\"Введите запрос\" maxlength=\"90\">"
		"<button>найти</button>"
		"</form>";
	std::wstring _a = L"<li><a href=\"https://";
	std::wstring _a_sred = L"\">";
	std::wstring _a_end = L"</a></li>";
	std::wstring stranica_suffix = L"</body></html>";
	std::wstring str = L"";
	std::string zapros = "select  host, path, size  from data d "
		"left join ref r on d.nomer_ref = r.id where ";
	pqxx::connection& bd;
	const int port;

	std::pair<std::string, std::string>Razbor_zaprosa(boost::asio::ip::tcp::socket& socket);//Разбираем запрос клиента	
	std::vector<std::string> Razbor_stroki(std::string str);//Разбор строки на слова 
	std::string Decoder(std::string input);//Декодируем непонятную кодировку в понятную
	std::string Sborka_zaprosa_bd(std::vector<std::string> str);//Формирование запроса для б/д
	std::string Obrabotka_zaprosa(std::pair<std::string, std::string>GetPost_Poisk, pqxx::connection& bd);//Обработка запроса и поиска
public:
	Server() = delete;
	Server(const Server&) = delete;
	Server(const Server&&) = delete;
	Server& operator=(const Server& other) = delete;
	Server& operator=(const Server&& other) = delete;
	Server(pqxx::connection& bd, const int port);
	void Start_Server();
};