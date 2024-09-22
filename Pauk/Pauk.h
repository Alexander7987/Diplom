#pragma once
#include <iostream>
#include <string>
#include <exception>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/url/parse.hpp>
#include <boost/locale.hpp>
#include <regex>
#include <queue>
#include <unordered_set>
#include <queue>
#include <map>
#include <pqxx/pqxx>

using std::string;
using namespace boost::beast;
using namespace boost::asio;
struct INI {	
	string start_sayt;
	string path;
	string port;
	int recursiya;	
};

class Pauk {
private:
	std::chrono::steady_clock::time_point start_;
	std::chrono::steady_clock::time_point end;

	//Первый пул
	int recursiya;
	void Task_Load_HTML(string host, const string path, const string port, int recursiya);//Метод для задачи на скачивание сайтов
	void Task_Load_BD(string html, string host, string path);//Метод для задачи по работе с базой(очитка html, загрузка ссылок и слов в базу)
	string Load_HTML(const string host, const string path, const string port);//Загрузка страниц
	std::pair<string, string> Razbor_Url_HTML(const string& url, string& Host);//Разбор URL на HOST и PATH
	std::vector<std::thread> Pool_HTML;//Пул потоков 
	std::queue<std::function<void()>> Tasks_HTML;//ОЧЕРЕДЬ на скачивание сайтов
	std::unordered_set<string> ref_HTML;//Проверка на повторяющиеся сайты
	int prev_ref_size_HTML = 0;//Для проверки на повторяющиеся сайты
	std::mutex m_HTML;
	bool Stop_Pool_HTML = true;
	void Thread_Pool_Load_HTML();//Метод пула

	//Второй пул
	std::vector<std::thread> Pool_BD;//Пул потоков 
	std::queue<std::function<void()>> Tasks_BD;//Задача для загрузки в базу
	std::mutex m_BD;
	bool Stop_Pool_BD = true;
	pqxx::connection& bd;//Подключение к базе
	std::map<string, int> Html_v_Slova_v_Map(string& html, int min_slovo, int max_slovo);//Метод очистки от всего кроме слов
	void Thread_Pool_Load_BD();//Метод пула

public:
	Pauk() = delete;
	Pauk(const Pauk&) = delete;
	Pauk(const Pauk&&) = delete;
	Pauk& operator=(const Pauk& other) = delete;
	Pauk& operator=(const Pauk&& other) = delete;
	Pauk(pqxx::connection& bd, const INI ini);
	~Pauk() {
		std::this_thread::sleep_for(std::chrono::seconds(recursiya*recursiya));
		Stop_Pool_HTML = false;
		for (auto& p : Pool_HTML)
			p.join();
		Stop_Pool_BD = false;
		for (auto& p : Pool_BD)
			p.join();
		end = chrono::steady_clock::now();
		std::cout << std::endl << "Затрачено времени: " << chrono::duration_cast<chrono::seconds>(end - start_).count() << std::endl;
	}
};
