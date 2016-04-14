#lang racket

(require net/http-client)
(require json)
(require racket/date)
(require web-server/servlet
         web-server/servlet-env)

(provide simdev-post-data
         format-date)

(define (keepalive hc devid)
  (http-conn-sendrecv! hc (format "/COMPANY/Devices/~a/Keepalive" devid)
                       #:method "POST"
                       #:data (jsexpr->string (hash 'Time "2015-8-8 19:50:23"))))

(define (device-status hc devid)
  (http-conn-sendrecv! hc (format "/COMPANY/Devices/~a/DeviceStatus" devid)
                       #:method "POST"
                       #:data (jsexpr->string (hash 'FaultState 0
                                                    'SenseCoilState 1
                                                    'FlashLightState 1
                                                    'IndicatoLightState 1))))

(define (format-date date)
  (format "~a-~a-~a ~a:~a:~a"
          (date-year date)
          (date-month date)
          (date-day date)
          (date-hour date)
          (date-minute date)
          (date-second date)))

(define (make-data)
  (bytes-append
   #"------------COMPANYMSG33a816d302b6\r\n"
   #"Content-Type: application/json;charset=UTF-8\r\n"
   (jsexpr->bytes
    (hash 'Longitude 101.2345
          'Latitude 101.1234
          'VehicleInfoState 0
          'IsPicUrl 0
          'LaneIndex 1
          'position (list 50 200 120 30)
          'direction 9
          'PlateInfo1 "鲁 B88888"
          'PlateInfo2 "鲁 B88888"
          'PlateColor 0
          'PlateType 1
          'PassTime (format-date (current-date))
          'VehicleSpeed 80.5
          'LaneMiniSpeed 40.0
          'LaneMaxSpeed 160.0
          'VehicleType 1
          'VehicleSubType 1
          'VehicleColor "A"
          'VehicleColorDepth 0
          'VehicleColorDepth 250
          'VehicleState 1
          'PicCount 2
          'PicType (list 1 2)
          'PlatePicUrl "http://192.168.1.100/platepic.jpg"
          'VehiclePic1Url "http://192.168.1.100/vehiclepic1.jpg"
          'VehiclePic2Url "http://192.168.1.100/vehiclepic2.jpg"
          'VehiclePic3Url "http://192.168.1.100/vehiclepic3.jpg"
          'CombinedPicUrl "http://192.168.1.100/combinedpic.jpg"
          'AlarmAction 1))
   #"------------COMPANYMSG33a816d302b6\r\n"
   #"Content-Type: image/jpeg\r\n"
   (file->bytes "test1.jpg")
   #"------------COMPANYMSG33a816d302b6\r\n"
   #"Content-Type: image/jpeg\r\n"
   (file->bytes "test.jpg")
   #"------------COMPANYMSG33a816d302b6--\r\n"))

(define (device-data hc devid data)
  (http-conn-sendrecv! hc (format "/COMPANY/Devices/~a/Datas" devid)
                       #:method "POST"
                       #:headers (list "Content-Type: multipart/form-data; boundary=----------COMPANYMSG33a816d302b6")
                       #:data data))

(define (simdev-post-data ip port devid data)
  (define hc (http-conn-open ip #:port port))
  (define-values (a b c) (device-data hc devid data))
  (http-conn-close! hc)
  (values a b c))

(define *record1* (hash 'Longitude 101.2345 'Latitude 101.1234 'VehicleInfoState 0 'IsPicUrl 0 'LaneIndex 1
                        'position (list 50 200 120 30) 'direction 9 'PlateInfo1 "鲁 B88888" 'PlateInfo2 "鲁 B88888"
                        'PlateColor 0 'PlateType 1 'PassTime "2015-08-19 10:00:00" 'VehicleSpeed 80.5 'LaneMiniSpeed 40.0
                        'LaneMaxSpeed 160.0 'VehicleType 1 'VehicleSubType 1 'VehicleColor "A" 'VehicleColorDepth 0
                        'VehicleColorDepth 250 'VehicleState 1 'PicCount 2 'PicType (list 1 2)
                        'PlatePicUrl "http://192.168.1.100/platepic.jpg"
                        'VehiclePic1Url "http://192.168.1.100/vehiclepic1.jpg"
                        'VehiclePic2Url "http://192.168.1.100/vehiclepic2.jpg"
                        'VehiclePic3Url "http://192.168.1.100/vehiclepic3.jpg"
                        'CombinedPicUrl "http://192.168.1.100/combinedpic.jpg"
                        'AlarmAction 1))

(define *record2* (hash 'Longitude 101.2345 'Latitude 101.1234 'VehicleInfoState 0 'IsPicUrl 0 'LaneIndex 1
                        'position (list 50 200 120 30) 'direction 9 'PlateInfo1 "鲁 C66666" 'PlateInfo2 "鲁 C66666"
                        'PlateColor 0 'PlateType 1 'PassTime "2015-08-19 10:00:30" 'VehicleSpeed 80.5 'LaneMiniSpeed 40.0
                        'LaneMaxSpeed 160.0 'VehicleType 1 'VehicleSubType 1 'VehicleColor "B" 'VehicleColorDepth 0
                        'VehicleColorDepth 250 'VehicleState 1 'PicCount 2 'PicType (list 1 2)
                        'PlatePicUrl "http://192.168.1.100/platepic.jpg"
                        'VehiclePic1Url "http://192.168.1.100/vehiclepic1.jpg"
                        'VehiclePic2Url "http://192.168.1.100/vehiclepic2.jpg"
                        'VehiclePic3Url "http://192.168.1.100/vehiclepic3.jpg"
                        'CombinedPicUrl "http://192.168.1.100/combinedpic.jpg"
                        'AlarmAction 1))

(define *record2-exit* (hash-set* *record2* 'direction 10 'PassTime "2015-08-19 13:00:30"))


(define (post-record ip port id record)
  (simdev-post-data ip port id
                    (bytes-append
                     #"------------COMPANYMSG33a816d302b6\r\n"
                     #"Content-Type: application/json;charset=UTF-8\r\n"
                     (jsexpr->bytes record)
                     #"------------COMPANYMSG33a816d302b6\r\n"
                     #"Content-Type: image/jpeg\r\n"
                     (file->bytes "test.jpg")
                     #"------------COMPANYMSG33a816d302b6\r\n"
                     #"Content-Type: image/jpeg\r\n"
                     (file->bytes "test1.jpg")
                     #"------------COMPANYMSG33a816d302b6--\r\n")))

;; (simdev-post-data "192.168.1.94" 8088 "0001" (make-data))

(define (format-time t)
  (format "~a-~a-~a ~a:~a:~a"
          (~a (date-year t) #:width 4 #:align 'right #:pad-string "0")
          (~a (date-month t) #:width 2 #:align 'right #:pad-string "0")
          (~a (date-day t) #:width 2 #:align 'right #:pad-string "0")
          (~a (date-hour t) #:width 2 #:align 'right #:pad-string "0")
          (~a (date-minute t) #:width 2 #:align 'right #:pad-string "0")
          (~a (date-second t) #:width 2 #:align 'right #:pad-string "0")))
 
(define (start req)
  (begin
    (post-record "192.168.1.86" 8088 "0001" *record2*)
    (post-record "192.168.1.86" 8088 "0002" *record2-exit*))
  (response/xexpr
   `(html (head (title "Ok!"))
          (body (p "Ok!")))))


(define (random-post-record ip port)
  (random-seed (date->seconds (current-date)))
  (define char-list "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
  (define (random-alphabet)
    (string-ref char-list (random 26)))
  (define (random-number)
    (string-ref char-list (+ 26 (random 10))))
  (define (random-record-in record-template)
    (hash-set* record-template
               'PassTime (format-time (current-date))
               'PlateInfo1 (format "鲁 B~a~a~a~a~a"
                                   (random-number)
                                   (random-alphabet)
                                   (random-alphabet)
                                   (random-number)
                                   (random-number))))
  (define (make-record-out record-in)
    (hash-set* record-in
               'PassTime (format-time (seconds->date (+ (date->seconds (current-date)) (random (* 8 3600)))))
               'direction 10))
  (define inner-records '())
  (define (random-post-record-in)
    (let loop ((record (random-record-in *record2*)))
      (sleep (random 60))
      (set! inner-records (cons record inner-records))
      (displayln record)
      (post-record ip port "0001" record)
      (loop (random-record-in *record2*))))
  (define (random-pick lst)
    (list-ref lst (random (length lst))))
  (define (random-post-record-out)
    (let loop ()
      (when (not (empty? inner-records))
        (let ((random-list (shuffle inner-records)))
          (displayln (make-record-out (first random-list)))
          (post-record ip port "0002" (make-record-out (first random-list)))
          (set! inner-records (cdr random-list))))
      (sleep (random 60))
      (loop)))
  (thread random-post-record-in)
  (thread random-post-record-out))
;; (serve/servlet start #:servlets-root "." #:listen-ip "0.0.0.0" #:port 8888 #:servlet-path "/")
