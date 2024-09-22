#pragma once
#include<string>
#include<fstream>
#include<vector>
#include<map>
#include<variant>
#include <iomanip>
#include <regex>
#include"syntax.h"

using Big_Map = std::map<string, std::map<string, std::variant<string, int, double>>>;
using Map = std::map<string, std::variant<string, int, double>>;
class ini_parser {
private:
	syntax stx;
	std::vector<string>str;
	Big_Map big_map;
	void load_data(string file) {
		std::ifstream fin(file);
		if (!fin.is_open())
			throw static_cast<string>("Файл отсутствует");
		string data; 
		while (getline(fin, data)) 
			razbor_str(data);
		fin.close();
		if(stx.ex)
			throw static_cast < string>("Исправьте файл");
		upakovka_Big_Map();
	}
	void razbor_str(string& data) {
		string perem = ""; string znach = "";
		data = std::regex_replace(data, std::regex("\\s+"), "");//data.erase(remove_if(data.begin(), data.end(), ::isspace), data.end());//убираем пробелы
		for (int i = 0; i < data.length(); ++i) {
			if (data[i] == ';')//убираем комментарии
				data.erase(i);
		}		
		if (data != "") {//Собираем вектор без пустых строк из файла
			stx.syntax_errors(data);
			stx.sekciya_perem_znach(data, perem, znach,'=');
			if (perem[0] == '[')
				str.push_back(perem);//пробелов между секцией и переменной не должно быть, для формата map
			else {
				str.push_back(perem);//если не секция укладываем переменная + значение
				str.push_back(znach);//даже если значение пустое,для формата map
			}			
		}		
	}	
	void upakovka_Big_Map() {//формат секция:переменная:значение
		int temp = 1;//смещение для переменных и значения от секции 
		for (int i = 0; i < str.size();) {
			if (str[i][0] != '[' && i == 0) {//если не секция  
				if (big_map.empty()) temp = 0;//смещение для переменных и значения если секции нет
				big_map["[]"][str[temp]] = get_variant(str[temp + 1]);
			}
			if (str[i][0] == '[') {
				if (i >= str.size() - 1) {// Пустая секция находится в конце файла
					big_map[str[i]][""] = "";
					return;
				}
				if (str[i + 1][0] == '[') {// Пустая секция находится везде кроме конца файла
					big_map[str[i]][""] = "";
					temp -= 2;
				}
				else big_map[str[i]][str[i + temp]] = get_variant(str[i + temp + 1]);
			}
			temp += 2;
			if ((i + temp) > str.size() - 1)//проверка выхода за границу вектора 
				return;
			if (str[i + temp][0] == '[') {
				i += temp;// вместо ++i в цикле
				temp = 1;
			}
		}
	}	
	std::variant<string, int, double> get_variant(string data) {//перевод строки в СТРОКУ в INT в DOUBLE		
		if (data[0] < 48 || data[0]>57)//если строка
			return data;
		int schetchik = 0;
		for (auto& x : data) {//Если типа хост то строка
			if(x == '.')
				if(++schetchik>1)
					return data;
		}
		if (data[0] >= 48 || data[0] <= 57) {//если чсло
			double temp = 0; int p = 1; bool db = false;
			for (auto& x : data) {//если точка то double db=true, (p) умножается на 10
				if (x != '.') {
					temp *= 10;
					temp = (temp + x - 48);
					if (db)
						p *= 10; 
				}
				else db = true;
			}
			if (db)
				return temp / p;
			else return static_cast<int>(temp);
		}
		return data;
	}
	void proverka_zaprosa(string sekciya, string perem) {
		bool sek = false; bool per = false;
		Big_Map::const_iterator big_ptr;
		for (big_ptr = big_map.begin(); big_ptr != big_map.end(); ++big_ptr)
			if (sekciya == big_ptr->first) {
				sek = true;
				break;
			}
		if (!sek)
			throw spisok_sekciy();
		for (Map::const_iterator ptr = big_ptr->second.begin(); ptr != big_ptr->second.end(); ++ptr)
			if (perem == ptr->first)
				per = true;
		if (!per)
			throw spisok_perem(big_ptr);
	}
	string spisok_sekciy() {
		string sek = "Секция отсутствует\nпосмотрите список существующих\n";
		for (Big_Map::const_iterator big_ptr = big_map.begin(); big_ptr != big_map.end(); ++big_ptr)
			sek = sek + big_ptr->first + "\n";
		return sek;
	}
	string spisok_perem(Big_Map::const_iterator big_ptr) {

		string perem = "В этой секции переменная отсутствует\nпосмотрите список существующих\n";
		for (Map::const_iterator ptr = big_ptr->second.begin(); ptr != big_ptr->second.end(); ++ptr) {
			if (ptr->first == "")
				return "В этой секции переменные отсутствует";
			perem = perem + ptr->first + "\n";
		}
		return perem;
	}

public:
	ini_parser() = delete;
	ini_parser(const ini_parser&) = delete;
	ini_parser& operator=(const ini_parser&) = delete;
	ini_parser(ini_parser&&) = delete;
	ini_parser& operator=(ini_parser&&) = delete;
	ini_parser(string file) {
		load_data(file);
	}
	template<class T>
	T get_value(string str) {		
		string sekciya = ""; string perem = "";
		stx.sekciya_perem_znach(str, sekciya, perem, '.');
		sekciya = "[" + sekciya + "]";
		proverka_zaprosa(sekciya, perem);
		string s = typeid(T).name();
		if (std::holds_alternative<int>(big_map.at(sekciya).at(perem))) {
			if (s != "int")
				throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"int\"";
			return  std::get<int>(big_map.at(sekciya).at(perem));
		}
		else if (std::holds_alternative<double>(big_map.at(sekciya).at(perem))) {
			if(s != "double")
				throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"double\"";
			return std::get<double>(big_map.at(sekciya).at(perem));
		}
		if(std::holds_alternative<string>(big_map.at(sekciya).at(perem)))
			if (std::get<string>(big_map.at(sekciya).at(perem)) == "")
				throw "В переменной \"" + perem + "\" значение отсутствует";
		throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"string\"";
	}
	template<>
	string get_value(string str) {		
		string sekciya = ""; string perem = "";
		stx.sekciya_perem_znach(str, sekciya, perem, '.');
		sekciya = "[" + sekciya + "]";
		proverka_zaprosa(sekciya, perem);
		if (std::holds_alternative<int>(big_map.at(sekciya).at(perem)))
			throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"int\"";
		if (std::holds_alternative<double>(big_map.at(sekciya).at(perem)))
			throw "Логическая ошибка, переменная \"" + perem + "\" имеет тип \"double\"";
		if (std::holds_alternative<string>(big_map.at(sekciya).at(perem)))
			if (std::get<string>(big_map.at(sekciya).at(perem)) != "")
				return  std::get<string>(big_map.at(sekciya).at(perem));
		throw "В переменной \"" + perem + "\" значение отсутствует";
	}	
	void print_am() {
		for (Big_Map::const_iterator big_ptr = big_map.begin(); big_ptr != big_map.end(); ++big_ptr) {
			std::cout << big_ptr->first << std::endl;
			for (Map::const_iterator ptr = big_ptr->second.begin(); ptr != big_ptr->second.end(); ++ptr) {
				if (ptr->first != "") {
					std::cout << ptr->first << " = ";
					if (std::holds_alternative<string>(ptr->second))
						std::cout << std::get<string>(ptr->second) << std::endl;
					if (std::holds_alternative<int>(ptr->second))
						std::cout << std::get<int>(ptr->second) << std::endl;
					if (std::holds_alternative<double>(ptr->second))
						std::cout <<std::fixed<< std::setprecision(1)<<std::get<double>(ptr->second) << std::endl;
				}
			}
		}
	}
};
