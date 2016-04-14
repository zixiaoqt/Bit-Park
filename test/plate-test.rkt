#lang racket/gui

;; (require ffi/unsafe
;;          racket/date)

;; (define liblev (ffi-lib "lev"))

;; (define matchscore
;;   (get-ffi-obj "matchscore" liblev (_fun _string _string -> _int)))

;; (define (random-plate-number)
;;   (define char-list "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789京津沪渝蒙新藏宁桂港澳黑吉辽晋冀青鲁豫苏皖浙闽赣湘鄂粤琼甘陕黔滇川")
;;   (define (random-alphabet)
;;     (string-ref char-list (random 26)))
;;   (define (random-number)
;;     (string-ref char-list (+ 26 (random 10))))
;;   (define (random-province)
;;     (string-ref char-list (+ 36 (random 33))))
;;   (format "~a ~a~a~a~a~a~a"
;;           (random-province)
;;           (random-alphabet)
;;           (random-number)
;;           (random-alphabet)
;;           (random-alphabet)
;;           (random-number)
;;           (random-number)))


;; (define (main)
;;   (random-seed (date->seconds (current-date)))

;;   (define plate-numbers (build-list 3000 (lambda (x) (random-plate-number))))
  
;;   ;; Make a frame by instantiating the frame% class
;;   (define frame (new frame% [label "Example"]
;;                      [width 1024]
;;                      [height 768]))

;;   (define list #f)
  
;;   (define text-field (new text-field%
;;                           (label "车牌号")
;;                           (parent frame)
;;                           (init-value "")
;;                           (callback
;;                            (lambda (t e)
;;                              (set! plate-numbers
;;                                (sort plate-numbers <
;;                                      #:key (lambda (v)
;;                                                (matchscore (send t get-value) v))
;;                                      #:cache-keys? #t))
;;                              (send list set plate-numbers)))))

;;   (set! list (new list-box%
;;                     (label "")
;;                     (choices plate-numbers)
;;                     (parent frame)))
;;   (send frame show #t)

;;   )

;; (main)

(require "plate-test-i.rkt"
         racket/date)

;; (define liblev (ffi-lib "lev"))

(define (main)
  (random-seed (date->seconds (current-date)))

  (define plate-numbers (build-list 10000 (lambda (x) (random-plate-number))))
  
  ;; Make a frame by instantiating the frame% class
  (define frame (new frame% [label "Example"]
                     [width 1024]
                     [height 768]))

  (define msg (new message% [label "info"]
                   [parent frame]))
  (define list #f)
  
  (define text-field (new text-field%
                          (label "车牌号")
                          (parent frame)
                          (init-value "")
                          (callback
                           (lambda (t e)
                             (when (equal? (send e get-event-type ) 'text-field-enter)
                               (set! plate-numbers
                                 (sort plate-numbers <
                                       #:key (lambda (v)
                                               (matchscore (send t get-value) v))
                                       #:cache-keys? #t))
                               (send list set plate-numbers))))))

  (set! list (new list-box%
                    (label "")
                    (choices plate-numbers)
                    (parent frame)))
  (send frame show #t)

  )

(main)
