#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <thread>

using namespace std;
using namespace std::chrono;

// Структура для хранения информации о стимуле
struct Stimulus {
    int variant;  // Вариант предъявления стимула (0, 1, 2)
    string content;  // Содержимое стимула (например, текст)
};

// Функция для генерации случайного числа
int getRandomNumber(int min, int max) {
    random_device rd;  
    mt19937 gen(rd()); 
    uniform_int_distribution<> distr(min, max);  
    return distr(gen);
}

int main() {
    // Вектор стимулов
    vector<Stimulus> stimuli = {
        {0, "Стимул 0"},  
        {1, "Стимул 1"},  
        {2, "Стимул 2"}   
    };

    // Пороговые значения задержек
    const int minDelay = 200;  
    const int maxDelay = 500;  
    const int maskDuration = 500; 

    // Основной цикл по каждому стимулу
    for (const auto& stim : stimuli) {
        cout << "Готовьтесь... " << endl;

        // Задержка перед предъявлением стимула
        int randomDelay = getRandomNumber(minDelay, maxDelay);
        this_thread::sleep_for(milliseconds(randomDelay));  // Пауза для имитации ожидания

        // Фиксируем время начала предъявления стимула
        auto start = high_resolution_clock::now();

        // Отображение стимула
        cout << "Предъявляется: " << stim.content << endl;

        // Если стимул требует маскирования
        if (stim.variant == 1) {
            this_thread::sleep_for(milliseconds(16));  // Время затухания стимула
            cout << "Маскер отображается..." << endl;
            this_thread::sleep_for(milliseconds(maskDuration));  // Время маскирования
        }

        // Ожидание нажатия кнопки или истечения времени
        cout << "Нажмите кнопку!" << endl;
        auto reactionStart = high_resolution_clock::now();
        string response;
        cin >> response;

        // Фиксируем время реакции
        auto reactionEnd = high_resolution_clock::now();
        auto reactionTime = duration_cast<milliseconds>(reactionEnd - reactionStart).count();

        // Проверка на ошибки
        if (reactionTime < 100) {
            cout << "Ошибка: Нажато слишком рано!" << endl;
        } else if (reactionTime > 1000) {
            cout << "Ошибка: Нажато слишком поздно!" << endl;
        } else if (stim.variant == 0 && !response.empty()) {
            cout << "Ошибка: Не надо было нажимать!" << endl;
        } else if (stim.variant == 2 && response.empty()) {
            cout << "Ошибка: Надо было нажать!" << endl;
        } else {
            cout << "Время реакции: " << reactionTime << " мсек." << endl;
        }

        // Пауза перед следующим стимулом
        this_thread::sleep_for(milliseconds(1000));
    }

    return 0;
}
