#include "Pauk.h"

Pauk::Pauk(pqxx::connection& bd, const INI ini):bd(bd),recursiya(ini.recursiya) {
    start_ = chrono::steady_clock::now(); //далее создание таблиц



    pqxx::work w{ bd };
    w.exec("create table if not exists ref(id serial PRIMARY KEY not null,host varchar ,path varchar )");
    w.exec("create table if not exists data(id serial not null, nomer_ref int  references ref (id)  not null, slovo varchar ,size int )");



    w.commit(); //перевод букв в нижний
    boost::locale::generator gen;
    std::locale loc = gen.generate("");
    std::locale::global(loc);
    std::wcout.imbue(loc); //далее загрузка первой задачи на скачивание сайта
    Tasks_HTML.push([ini, this]() { Task_Load_HTML(ini.start_sayt, ini.path, ini.port, recursiya); });
    for (int i = 0; i < std::thread::hardware_concurrency()/2; ++i) {
        Pool_HTML.emplace_back(&Pauk::Thread_Pool_Load_HTML, this);
        Pool_BD.emplace_back(&Pauk::Thread_Pool_Load_BD, this);
    }
}

//////Первый пул. Задача для загрузки страниц и задачи по работе сбазой
void Pauk::Task_Load_HTML(string host, string path, const string port, int recursiya) {
    const string html = Load_HTML(host, path, port);//Грузим страницу
    if (html.empty())
        return;
   



    m_HTML.lock();
    Tasks_BD.push([html, host, path,this]() {
        Pauk::Task_Load_BD(html,host,path); });//Грузим задачу для работы с базой
    static int x = 0;//Для инфо в консоли
    std::cout << "Сколько сайтов  " << ++x << "  ID Потока  " << std::this_thread::get_id() << "  Tasks  " << Tasks_HTML.size() << std::endl; 
    m_HTML.unlock(
    );
      


    if (--recursiya == 0)
        return;
    std::smatch match;
    std::string::const_iterator Html_Start(html.cbegin());
    while (std::regex_search(Html_Start, html.cend(), match, std::regex("<a href=\"(.*?)\""))) {//Берем ссылки других сайтов      
        auto [Host, Path] = Razbor_Url_HTML(match[1], host);//Разбираем ссылки на Host и Path
        Html_Start = match.suffix().first;
        m_HTML.lock();
        ref_HTML.insert(Host + Path);
        int size = ref_HTML.size();
        int prev = prev_ref_size_HTML;
        m_HTML.unlock();
        if (prev == size)//повторяющиеся сайты
            continue;
        m_HTML.lock();
        prev_ref_size_HTML = size;
        Tasks_HTML.push([Host, Path, port, recursiya, this]() {
            Pauk::Task_Load_HTML(Host, Path, port, recursiya); });
        m_HTML.unlock();
    }
}




//Загрузка страниц
string Pauk::Load_HTML(const string host, const string path, const string port) {
    string html;
    try {
        io_service servis;
        ssl::context context(ssl::context::sslv23_client);
        context.set_default_verify_paths();
        ssl::stream<ip::tcp::socket> ssocket = { servis, context };
        ssocket.set_verify_mode(ssl::verify_none);
        ip::tcp::resolver resolver(servis);
        auto it = resolver.resolve(host, port);
        connect(ssocket.lowest_layer(), it);
        ssocket.handshake(ssl::stream_base::handshake_type::client);
        http::request<http::string_body> req{ http::verb::get, path, 11 };
        req.set(http::field::host, host);
        m_HTML.lock();
        http::write(ssocket, req);
        m_HTML.unlock();
        http::response<http::string_body> res;
        flat_buffer buffer;
        http::read(ssocket, buffer, res);
        html = res.body().data();
    }
    catch (const std::exception& e) {
       // std::cout << e.what() << std::endl;
    }
    return html;
}

////Разбираем ссылки на Host и Path
std::pair<string, string> Pauk::Razbor_Url_HTML(const string& url,  string& Host) {
    string host, path;
    try {
        boost::urls::url_view URL(url);
        host = URL.host(); path = URL.path();
        if (host.empty())
            host = Host;
        else Host = host;
        if (path.empty())
            path = "/";
    }
    catch (const std::exception& e) {//Исключения обрабатываем здесь, если уровнем выше обрабатывать, 50% сайтов пропустит
        //std::cout << e.what() << std::endl;
    }    
    return std::make_pair(host, path);
}

//Первый пул потоков
void Pauk::Thread_Pool_Load_HTML() {
    while (Stop_Pool_HTML || !Tasks_HTML.empty()) {
        int x = Tasks_HTML.size();
        if (x < 10)//синхронизация потоков при старте,чтоб не словить Stop_Pool_HTML
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 5 * 100));
        m_HTML.lock();
        if (!Tasks_HTML.empty()) {
            auto task = Tasks_HTML.front();
            Tasks_HTML.pop();
            m_HTML.unlock();
            task();
        }
        else { 
            m_HTML.unlock(); }
    }
}


//////Блок второго пула
//Задача для очистки строки и загрузки в базу
void Pauk::Task_Load_BD(string html, string host, string path) {
    static int count = 0;
    try {
        auto slova = Html_v_Slova_v_Map(html, 3, 15);//Очистка строки
        m_BD.lock();
        ++count;//метка для таблицы слов(указывает на ссылку в таблице ссылок)
        std::cout << "Количесво загрузок  " << count << "  ID Потока  " << std::this_thread::get_id() << "  Tasks_BD  " << Tasks_BD.size() << std::endl;
        pqxx::work w{ bd };
        w.exec("insert into ref(host, path) values ('" + host + "', '" + path + "')");//Загрузка ссылок в базу
        for (auto [key, size] : slova) {
            w.exec("insert into data(nomer_ref, slovo, size) values (" + std::to_string(count) + ", '" + key + "', " + std::to_string(size) + ")");//Загрузка слов в базу
        }//Если хоть одна из w.exec() выбросит исключение загрузки не будет, что позволяет не попутать страницы и ссылки.
        w.commit();
        m_BD.unlock();
    }
    catch (const std::exception& e) {
        //std::cout << e.what() << std::endl;
        m_BD.unlock();
        --count;
    }
}

//Очистка строк с HTML кодом
std::map<string, int> Pauk::Html_v_Slova_v_Map(string& html, int min_slovo, int max_slovo) {
    //УБИРАЕМ символы < > и т.п.
    bool trigger = false;
    for (int i = 0; i < html.size(); ++i) {
        if (html[i] == '<')
            trigger = true;
        if (html[i] == '>')
            trigger = false;
        if (trigger) {
            html.erase(i, 1); --i;
        }
    }


    for (int i = 0; i < html.size(); ++i) {//Убираем все, что не буквы
        unsigned char z = html[i];
        if (z < 32 || (z > 32 && z < 65) || (z > 90 && z < 97) || (z > 122 && z < 128)) {
            html[i] = ' ';
        }
    }


    for (int i = 0; i < html.size() - 1; ++i) {//Убираем пробелы			
        if (html[i] == ' ' && html[i + 1] == ' ') {
            html.erase(i, 1);
            --i;
        }
    }


    if (html[0] == ' ')//Убираем первый пробел
        html.erase(0, 1);

    html = boost::locale::to_lower(html);//Переводим в нижний регистр(для этой штуки в конструкторе код)


    std::map<string, int>map;
    string temp;
    for (auto& x : html) {// Разбираем строку и суем в мар
        int temp_min, temp_max;
        if (x != ' ') {
            temp += x;
        }
        else {
            //Здесь убираем слова меньше 3(6 Рус) и больше 15(30 Рус) символов.
            if (unsigned(temp[0]) >= 208) {//Эта сторчка для пониманя Русская буква или латинская
                temp_min = min_slovo * 2;  temp_max = max_slovo * 2;//Если Русская удаляем символов в два раза больше
            }
            else {
                temp_min = min_slovo; temp_max = max_slovo;//Если латинская, то как есть
            }
            if (temp.size() <= temp_min) {
                temp = ""; continue;
            }
            if (temp.size() > temp_max) {
                temp = ""; continue;
            }


            map[temp]++;
            temp = "";
        }
    };   
    return map;
};

void Pauk::Thread_Pool_Load_BD() {//Второй пул потоков
    while (Stop_Pool_BD || !Tasks_BD.empty()) {
        int x = Tasks_BD.size();
        if (x < 20)//синхронизация потоков при старте, чтоб не словить Stop_Pool_BD тормозит на финише
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 5 * 100));
        m_BD.lock();
        if (!Tasks_BD.empty()) {
            auto task = Tasks_BD.front();
            Tasks_BD.pop();
            m_BD.unlock();
            task();
        }
        else { m_BD.unlock(); }
    }
}
