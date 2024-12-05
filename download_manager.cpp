//
// Created by Glauco A. Colaco on 03/12/24.
//
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <curl/curl.h>

using namespace std;

mutex output_mutex; // Para sincronizar a saída no console

class DownloadManager {
public:
    static size_t write_data(void *ptr, size_t size, size_t nitems, FILE *stream) {
        return fwrite(ptr, size, nitems, stream);
    }

    static void download_file(const string& url, const string& output_file) {
        CURL* curl;
        CURLcode res;
        FILE* file;

        curl = curl_easy_init();
        if(curl) {
            file = fopen(output_file.c_str(), "wb");
            if(!file) {
                lock_guard<mutex> lock(output_mutex);
                cerr << "Erro ao abrir arquivo para escrita: " << output_file << "\n";
                return;
            }
        } else {
            lock_guard<std::mutex> lock(output_mutex);
            cerr << "Erro ao inicializar libcurl\n";
        }

        //Configurar libcurl
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

        //Fazer o download
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            lock_guard<mutex> lock(output_mutex);
            cerr << "Erro ao baixar " << url << ": " << curl_easy_strerror(res);
        } else {
            lock_guard<mutex> lock(output_mutex);
            cout << "Download concluido: " << url << " -> " << output_file << "\n";
        }

        fclose(file);
        curl_easy_cleanup(curl);
    }
};

int main() {
    vector<pair<string, string> > downloads = {
            {"https://www.w3.org/WAI/WCAG21/quickref/quickref.zip", "quickref.zip"},
            {"https://raw.githubusercontent.com/microsoft/cpprestsdk/master/Release/2.10.18/vcpkg/versions.json", "versions.json"},
            {"https://raw.githubusercontent.com/tensorflow/tensorflow/master/LICENSE", "tensorflow_license.txt"},
            {"https://www.gnu.org/licenses/lgpl-3.0.txt", "lgpl_license.txt"}
    };

    vector<thread> threads;

    curl_global_init(CURL_GLOBAL_ALL);

    threads.reserve(downloads.size());
    for(const auto& download : downloads) {
        threads.emplace_back(DownloadManager::download_file, download.first, download.second);
    }

    for(auto& t : threads) {
        if(t.joinable()) {
            t.join();
        }
    }

    //Limpar o libcurl
    curl_global_cleanup();

    cout << "Todos os downloads foram concluidos com sucesso!\n";
    return 0;
}
