# MyLisp

Small research project of mine: A LISP! 

Because you have to, right?

## Compile

make all

## Run

Example run of the awesome capabilities:

```
[markus@m1ndbl0w mylisp]$ make && valgrind ./mylisp 
ALL STARTUP TESTS PASSED!

Welcome to MyLisp.
>> (+ 5 3)
[n] 8
>> (cons 'alice 'bob)
[c] ('alice 'bob)
>> (car (cons 'alice 'bob))
[s] 'alice
>> (cdr (cons 'alice 'bob))
[s] 'bob
>> (cons 'M (cons 'y (cons 'L (cons 'i (cons 's (cons 'p))))))
[c] ('M 'y 'L 'i 's 'p)
>> (quote (cons 'M (cons 'y (cons 'L (cons 'i (cons 's (cons 'p)))))))
[c] (cons 'M (cons 'y (cons 'L (cons 'i (cons 's (cons 'p))))))
>> (eval (quote (cons 'M (cons 'y (cons 'L (cons 'i (cons 's (cons 'p))))))))
[c] ('M 'y 'L 'i 's 'p)
>> (dump)
nil
>> (set 'x 10)
[n] 10
>> (+ x 3)
[n] 13
>> (dump)
x				[n] 10
nil
>> (quote (+ x y))
[c] (+ x y)
>> (set 'y 5)
[n] 5
>> (dump)
x				[n] 10
y				[n] 5
nil
>> (eval (quote (+ x y)))
[n] 15
>> (list 5 4 3 2 1)
[c] (5 4 3 2 1)
>> (nth 3 (list 5 4 3 2 1))
[n] 2
>> (set 'f (quote (nth idx (list 1 2 3 4 5 6 7 8))))
[c] (nth idx (list 1 2 3 4 5 6 7 8))
>> (dump)
f				[c] (nth idx (list 1 2 3 4 5 6 7 8))
x				[n] 10
y				[n] 5
nil
>> (set 'idx 4)
[n] 4
>> (eval f)
[n] 5
>> 
```
