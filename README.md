# cpp-search-server

### Модули

- [search_server](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/search_server.h) (Поисковая машина)
- [document](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/document.h) (Модель документа)
- [remove_duplicates](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/remove_duplicates.h) (Дедупликатор документов)
- [request_queue](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/request_queue.h) (Анализ запросов)
- [paginator](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/paginator.h) (Разбиение результатов на страницы)
- [iterator](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/iterator.h) (Работа с итераторами)
- [string_processing](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/string_processing.h) (Работа со строками)
- [read_input_functions](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/read_input_functions.h) (Ввод данных)
- [output_functions](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/output_functions.h) (Вывод данных)
- [log_duration](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/log_duration.h) (Подсчет времени выполнения кода)
- [stream_uniter](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/stream_uniter.h) (Управление потоками ввода/вывода)
- [test_example_functions](https://github.com/AlexeyShalaev/cpp-search-server/blob/main/search-server/test_example_functions.h) (Тестирование системы)

### Примечание
Поиск осуществляется только по полному сопадению слова, поэтому данное решение лучше использовать, если у вас много документов и запросы довольно точные.
