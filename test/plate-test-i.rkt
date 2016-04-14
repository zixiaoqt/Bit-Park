#lang typed/racket

(require math/array)
(provide matchscore
         random-plate-number)

(: matchscore (-> String String Integer))
(define (matchscore s t)
  (let* ((m (string-length s))
         (n (string-length t))
         (d (array->mutable-array (make-array (vector (+ m 1) (+ n 1)) 0))))
    (for ((i (in-range 0 (+ m 1))))
      (array-set! d (vector i 0) i))
    (for ((j (in-range 1 (+ n 1))))
      (array-set! d (vector 0 j) j))
    (for ((j (in-range 1 (+ n 1))))
      (for ((i (in-range 1 (+ m 1))))
        (if (char=? (string-ref s (- i 1)) (string-ref t (- j 1)))
            (array-set! d (vector i j) (array-ref d (vector (- i 1) (- j 1))))
            (array-set! d (vector i j)
                        (+ (min (array-ref d (vector (- i 1) j))
                                                  (min
                                                   (array-ref d (vector i (- j 1)))
                                                   (array-ref d (vector (- i 1) (- j 1)))))
                                    1)))))
    ;; (pretty-display d)
    (array-ref d `#(,m ,n))))

;; (define matchscore
;;   (get-ffi-obj "matchscore" liblev (_fun _string _string -> _int)))

(: random-plate-number (-> String))
(define (random-plate-number)
  (define char-list "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789京津沪渝蒙新藏宁桂港澳黑吉辽晋冀青鲁豫苏皖浙闽赣湘鄂粤琼甘陕黔滇川")
  (define (random-alphabet)
    (string-ref char-list (random 26)))
  (define (random-number)
    (string-ref char-list (+ 26 (random 10))))
  (define (random-province)
    (string-ref char-list (+ 36 (random 33))))
  (format "~a ~a~a~a~a~a~a"
          (random-province)
          (random-alphabet)
          (random-number)
          (random-alphabet)
          (random-alphabet)
          (random-number)
          (random-number)))
