# Лабораторная работа №5

Производитель-потребитель на потоках. Синхронизация при помощи семофоров и мьютекса 
___

### Для запуска:
> *$* make run
___

### Руководство пользователя

Для увеличения или уменюшения очереди использовать "*+*" и "*-*" соответственно

Для создания процесса производителя использовать "*p*"

Для удаления последнего созданного производителя использовать "*kp*"

Для удаления всех производителей использовать "*kap*"

Для создания процесса потребителя использовать "*с*"

Для удаления последнего созданного потребителя использовать "*kc*"

Для удаления всех потребителей использовать  "*kaс*"

Удалить все процессы "*ka*"

Выйти "*q*" 
___

### Интерестные факты

- Присутствует глобальный массив флагов *Run*, который ничего не делает, в принципе. Можно попробовать убрать

- При неудачном уменьшении очереди виснет. Например: производители заполнили всю очередь и ждут пока освободится, а мы уменьшаем
___

P.S. Часть с условными переменными реализованна в [osisp-7.1](https://github.com/0rpheus-0/osisp-7.1)