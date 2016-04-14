#lang racket

(require racket/tcp)
(require json)

(provide dtclient-test
         dtclient-loop-forever)

(define (read-until in delim)
  (define (read-until% in delim s)
    (let ((c (read-char in)))
      (cond 
       ((eof-object? c) s)
       ((equal? c delim) s)
       (else
        (read-until% in delim (string-append s (string c)))))))
  (let ((s ""))
    (read-until% in delim s)))

(define (checksum bytes)
  (define (checksumi bytes i sum)
    (if (= i (bytes-length bytes))
        sum
        (checksumi bytes (+ i 1) (+ sum (bytes-ref bytes i)))))
  (checksumi bytes 0 0))

(define (dtclient-test ip port)
  (define-values (in out) (tcp-connect ip port))
  (displayln (read-until in #\,));ipnc
  (displayln (read-until in #\,));addr
  (displayln (read-until in #\,));type
  (displayln (read-until in #\,));sum
  (define len (string->number (read-until in #\,)))
  (displayln len);length
  (displayln (read-string len in))
  (flush-output (current-output-port))
  (define response (jsexpr->string (hash 'ret 0 'desc "desc")))
  (displayln (format "IPNC,,100,~a,~a,~a"
                        (checksum (string->bytes/utf-8 response))
                        (string-length response)
                        response))
  (write-string (format "IPNC,,100,~a,~a,~a"
                        (checksum (string->bytes/utf-8 response))
                        (string-length response)
                        response)
                out)
  (flush-output out)
  (close-input-port in)
  (close-output-port out))

(define (dtclient-loop-forever ip port)
  (define-values (in out) (tcp-connect ip port))
  (let loop ()
    (displayln (read-until in #\,));ipnc
    (displayln (read-until in #\,));addr
    (displayln (read-until in #\,));type
    (displayln (read-until in #\,));sum
    (define len (string->number (read-until in #\,)))
    (displayln len);length
    (displayln (read-bytes len in))
    (flush-output (current-output-port))
    (define response (jsexpr->string (hash 'ret 0 'desc "desc")))
    (displayln (format "IPNC,,100,~a,~a,~a"
                       (checksum (string->bytes/utf-8 response))
                       (string-length response)
                       response))
    (write-string (format "IPNC,,100,~a,~a,~a"
                          (checksum (string->bytes/utf-8 response))
                          (string-length response)
                          response)
                  out)
    (flush-output out)
    (loop))
  (close-input-port in)
  (close-output-port out))

;; (dtclient-loop-forever "192.168.1.94" 2015)
