#lang racket

(require rackunit
         db
         json
         racket/future
         racket/date
         "client.rkt"
         "dtclient.rkt"
         "simdev.rkt"
         (for-syntax racket/match)
         (for-syntax racket/list))

(define *server-ip* "192.168.1.86")
(define *server-http-port* 8088)
(define *server-port* 9000)
(define *dts-port* 2015)
(define *device1-ip* "192.168.1.86")
(define *device2-ip* "192.168.1.95")
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

(define-for-syntax (make-match-record ids records)
  (map (lambda (id record)
         #`(hash-table ((quote DeviceID) (? (equal-val? #,id)))
                       ((quote PassID) _)
                       ((quote PlateType) (? (equal-val? (hash-ref #,record 'PlateType))))
                       ((quote PassTime) (? (equal-val? (hash-ref #,record 'PassTime))))
                       ((quote VehicleType) (? (equal-val? (hash-ref #,record 'VehicleType))))
                       ((quote VehicleSubType) (? (equal-val? (hash-ref #,record 'VehicleSubType))))
                       ((quote VehicleColor) (? (equal-val? (hash-ref #,record 'VehicleColor))))
                       ((quote PassDirection) (? (equal-val? (hash-ref #,record 'direction))))
                       ((quote PictureURL1) (? (equal-val? (format "http://~a:~a/PassVehicle/~a_~a_vehiclepic1.jpg"
                                                                   *server-ip*
                                                                   *server-http-port*
                                                                   #,id
                                                                   (string-remove(hash-ref #,record 'PassTime)
                                                                                 "-" ":" " ")))))
                       ((quote PictureURL2) (? (equal-val? (format "http://~a:~a/PassVehicle/~a_~a_vehiclepic2.jpg"
                                                                   *server-ip*
                                                                   *server-http-port*
                                                                   #,id
                                                                   (string-remove(hash-ref #,record 'PassTime)
                                                                                 "-" ":" " ")))))
                       ((quote PlateNumber1) (? (equal-val? (hash-ref #,record 'PlateInfo1))))
                       ((quote PlateNumber2) (? (equal-val? (hash-ref #,record 'PlateInfo2))))
                       ((quote PlateColor) (? (equal-val? (hash-ref #,record 'PlateColor))))
                       ((quote PlatePosition) (? (equal-val? (hash-ref #,record 'position))))))
       ids records))

(define (equal-val? val)
  (lambda (v) (equal? val v)))

(define (string-remove str . s)
  (if (empty? s)
      str
      (apply string-remove (string-replace str (car s) "") (cdr s))))

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

(define (check-enter-vehicle-notify val id record)
    (check-match val
                 (hash-table ((quasiquote CmdType) "EnterVehicleNotify")
                             ((quasiquote PassID) passid)
                             ((quasiquote PlateType) (? (equal-val? (hash-ref record 'PlateType))))
                             ((quasiquote PlateNumber1) (? (equal-val? (hash-ref record 'PlateInfo1))))
                             ((quasiquote PlateNumber2) (? (equal-val? (hash-ref record 'PlateInfo2))))
                             ((quasiquote DeviceID) (? (equal-val? id)))
                             ((quasiquote PassDirection) (? (equal-val? (hash-ref record 'direction))))
                             ((quasiquote PictureURL1) (? (equal-val? (format "http://~a:~a/PassVehicle/~a_~a_vehiclepic1.jpg"
                                                                              *server-ip*
                                                                              *server-http-port*
                                                                              id
                                                                              (string-remove(hash-ref record 'PassTime)
                                                                                            "-" ":" " ")))))
                             ((quasiquote PictureURL2) (? (equal-val? (format "http://~a:~a/PassVehicle/~a_~a_vehiclepic2.jpg"
                                                                              *server-ip*
                                                                              *server-http-port*
                                                                              id
                                                                              (string-remove(hash-ref record 'PassTime)
                                                                                            "-" ":" " ")))))
                             ((quote PlateColor) (? (equal-val? (hash-ref record 'PlateColor))))
                             ((quasiquote VehicleColor) (? (equal-val? (hash-ref record 'VehicleColor))))
                             ((quasiquote VehicleType) (? (equal-val? (hash-ref record 'VehicleType))))
                             ((quasiquote VehicleSubType) (? (equal-val? (hash-ref record 'VehicleSubType))))
                             ((quasiquote PassTime) (? (equal-val? (hash-ref record 'PassTime))))
                             ((quasiquote PlatePosition) (? (equal-val? (hash-ref record 'position)))))))

(define (check-exit-vehicle-notify val id record-exit [record-enter #f] [parking-type 9] [total-charge 0] [require-charge 0])
    (if record-enter
        (check-match
         val
         (hash-table
          ((quote CmdType) "ExitVehicleNotify")
          ((quote ExitVehicle) (hash-table ((quote PassID) _)
                                           ((quote PlateType) (? (equal-val? (hash-ref record-exit 'PlateType))))
                                           ((quote PlateNumber1) (? (equal-val? (hash-ref record-exit 'PlateInfo1))))
                                           ((quote PlateNumber2) (? (equal-val? (hash-ref record-exit 'PlateInfo2))))
                                           ((quote DeviceID) (? (equal-val? id)))
                                           ((quote PassDirection) (? (equal-val? (hash-ref record-exit 'direction))))
                                           ((quote PictureURL1) (? (equal-val?
                                                                    (format "http://~a:~a/PassVehicle/~a_~a_vehiclepic1.jpg"
                                                                            *server-ip*
                                                                            *server-http-port*
                                                                            id
                                                                            (string-remove (hash-ref record-exit 'PassTime)
                                                                                           "-" ":" " ")))))
                                           ((quote PictureURL2) (? (equal-val?
                                                                    (format "http://~a:~a/PassVehicle/~a_~a_vehiclepic2.jpg"
                                                                            *server-ip*
                                                                            *server-http-port*
                                                                            id
                                                                            (string-remove (hash-ref record-exit 'PassTime)
                                                                                           "-" ":" " ")))))
                                           ((quote PlateColor) (? (equal-val? (hash-ref record-exit 'PlateColor))))
                                           ((quote VehicleColor) (? (equal-val? (hash-ref record-exit 'VehicleColor))))
                                           ((quote VehicleType) (? (equal-val? (hash-ref record-exit 'VehicleType))))
                                           ((quote VehicleSubType) (? (equal-val? (hash-ref record-exit 'VehicleSubType))))
                                           ((quote PassTime) (? (equal-val? (hash-ref record-exit 'PassTime))))
                                           ((quasiquote PlatePosition) (? (equal-val? (hash-ref record-exit 'position))))))
          ((quote EnterVehicle) (hash-table ((quote PassID) _)
                                            ((quote PlateType) (? (equal-val? (hash-ref record-enter 'PlateType))))
                                            ((quote PlateNumber1) (? (equal-val? (hash-ref record-enter 'PlateInfo1))))
                                            ((quote PlateNumber2) (? (equal-val? (hash-ref record-enter 'PlateInfo2))))
                                            ((quote DeviceID) (? (equal-val? id)))
                                            ((quote PassDirection) (? (equal-val? (hash-ref record-enter 'direction))))
                                            ((quote PictureURL1) (? (equal-val?
                                                                     (format "http://~a:~a/PassVehicle/~a_~a_vehiclepic1.jpg"
                                                                             *server-ip*
                                                                             *server-http-port*
                                                                             id
                                                                             (string-remove(hash-ref record-enter 'PassTime)
                                                                                           "-" ":" " ")))))
                                            ((quote PictureURL2) (? (equal-val?
                                                                     (format "http://~a:~a/PassVehicle/~a_~a_vehiclepic2.jpg"
                                                                             *server-ip*
                                                                             *server-http-port*
                                                                             id
                                                                             (string-remove(hash-ref record-enter 'PassTime)
                                                                                           "-" ":" " ")))))
                                            ((quote PlateColor) (? (equal-val? (hash-ref record-enter 'PlateColor))))
                                            ((quote VehicleColor) (? (equal-val? (hash-ref record-enter 'VehicleColor))))
                                            ((quote VehicleType) (? (equal-val? (hash-ref record-enter 'VehicleType))))
                                            ((quote VehicleSubType) (? (equal-val? (hash-ref record-enter 'VehicleSubType))))
                                            ((quote PassTime) (? (equal-val? (hash-ref record-enter 'PassTime))))
                                            ((quasiquote PlatePosition) (? (equal-val? (hash-ref record-exit 'position))))))
          ((quote ParkingType) (? (equal-val? parking-type)))
          ((quote ChargeID) _)
          ((quote TotalCharge) (? (equal-val? total-charge)))
          ((quote RequireCharge) (? (equal-val? require-charge)))
          ((quote Bill) (hash-table ((quote EnterTime) (? (equal-val? (hash-ref record-enter 'PassTime))))
                                    ((quote ExitTime) (? (equal-val? (hash-ref record-exit 'PassTime))))
                                    ((quote Duration) _)
                                    ((quote FreeDuration) _)
                                    ((quote BeforeDiscountedFee) _)
                                    ((quote TotalFee) _)
                                    ((quote Discount) _)
                                    ((quote List) _)))))
        (check-match
         val
         (hash-table ((quote CmdType) "ExitVehicleNotify")
                     ((quote ExitVehicle)
                      (hash-table ((quote DeviceID) (? (equal-val? id)))
                                  ((quote PassDirection) (? (equal-val? (hash-ref record-exit 'direction))))
                                  ((quote PassID) passid)
                                  ((quote PassTime) (? (equal-val? (hash-ref record-exit 'PassTime))))
                                  ((quote PictureURL1) (? (equal-val?
                                                           (format "http://~a:~a/PassVehicle/~a_~a_vehiclepic1.jpg"
                                                                   *server-ip*
                                                                   *server-http-port*
                                                                   id
                                                                   (string-remove(hash-ref record-exit 'PassTime)
                                                                                 "-" ":" " ")))))
                                  ((quote PictureURL2) (? (equal-val?
                                                           (format "http://~a:~a/PassVehicle/~a_~a_vehiclepic2.jpg"
                                                                   *server-ip*
                                                                   *server-http-port*
                                                                   id
                                                                   (string-remove(hash-ref record-exit 'PassTime)
                                                                                 "-" ":" " ")))))
                                  ((quote PlateNumber1) (? (equal-val? (hash-ref record-exit 'PlateInfo1))))
                                  ((quote PlateNumber2) (? (equal-val? (hash-ref record-exit 'PlateInfo2))))
                                  ((quote PlateType) (? (equal-val? (hash-ref record-exit 'PlateType))))
                                  ((quote PlateColor) (? (equal-val? (hash-ref record-exit 'PlateColor))))
                                  ((quote VehicleColor) (? (equal-val? (hash-ref record-exit 'VehicleColor))))
                                  ((quote VehicleSubType) (? (equal-val? (hash-ref record-exit 'VehicleSubType))))
                                  ((quote VehicleType) (? (equal-val? (hash-ref record-exit 'VehicleType))))
                                  ((quasiquote PlatePosition) (? (equal-val? (hash-ref record-exit 'position))))))))))

(define (test)
  (test-begin
   (define (db-clear-table dbconn table)
    (query-exec dbconn (format "truncate table ~a" table)))

  (define (db-clear dbconn)
    (map (lambda (table)
           (db-clear-table dbconn table))
         (list-tables dbconn)))

  (define (db-insert-rule dbconn
                          #:nid                   [nid                  "rule1"]
                          #:name                  [name                 "rule1"]
                          #:rule_priority         [rule_priority        2]
                          #:festival_begin_time   [festival_begin_time  sql-null]
                          #:festival_end_time     [festival_end_time    sql-null]
                          #:day_begin             [day_begin            "09:00"]
                          #:night_begin           [night_begin          "21:00"]
                          #:free_time_length      [free_time_length     0]
                          #:dayfeespan            [dayfeespan           30]
                          #:dayfeerate            [dayfeerate           0]
                          #:dayfeespanbig         [dayfeespanbig        30]
                          #:dayfeeratebig         [dayfeeratebig        0]
                          #:nightfeespan          [nightfeespan         30]
                          #:nightfeerate          [nightfeerate         0]
                          #:nightfeespanbig       [nightfeespanbig      30]
                          #:nightfeeratebig       [nightfeeratebig      0]
                          #:maxfeebig             [maxfeebig            0]
                          #:maxfee                [maxfee               0]
                          #:free_time_sections    [free_time_sections   sql-null]
                          #:park_position         [park_position        0]
                          #:week_day              [week_day             sql-null]
                          #:create_user           [create_user          sql-null]
                          #:create_time           [create_time          sql-null]
                          #:invalid_flag          [invalid_flag         1]
                          #:invalid_time          [invalid_time         sql-null])
    (query-exec dbconn (string-append
                        "replace into park_rule_config"
                        " (nid,name,rule_priority,festival_begin_time,festival_end_time"
                        " ,day_begin,night_begin,free_time_length,dayfeespan,dayfeerate"
                        " ,dayfeespanbig,dayfeeratebig,nightfeespan,nightfeerate,nightfeespanbig"
                        " ,nightfeeratebig,maxfeebig,maxfee,free_time_sections,park_position"
                        " ,week_day,create_user,create_time,invalid_flag,invalid_time)"
                        " values "
                        " (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)")
                nid name rule_priority festival_begin_time festival_end_time
                day_begin night_begin free_time_length dayfeespan dayfeerate
                dayfeespanbig dayfeeratebig nightfeespan nightfeerate nightfeespanbig
                nightfeeratebig maxfeebig maxfee free_time_sections park_position
                week_day create_user create_time invalid_flag invalid_time))

  (define (db-insert-vip dbconn
                         #:vip_id [vip_id "vip_id1"]
                         #:carno [carno "鲁 B22222"]
                         #:carno_type [carno_type "02"]
                         #:name [name "name1"]
                         #:sex [sex 1]
                         #:age [age 29]
                         #:licence_type [licence_type 1]
                         #:licence_no [licence_no "370xxxxxxxxxxxxxxx"]
                         #:phone [phone "187xxxxxxxx"]
                         #:carcolor [carcolor "B"]
                         #:brand [brand 1]
                         #:brand_sub_type [brand_sub_type 1]
                         #:vip_type [vip_type 1]
                         #:expiry_date [expiry_date sql-null]
                         #:remain_money [remain_money 0]
                         #:valid_scope [valid_scope sql-null]
                         #:delete_status [delete_status 1]
                         #:freeze_status [freeze_status 1]
                         #:freeze_until [freeze_until sql-null]
                         #:vip_discount [vip_discount sql-null])
    (query-exec
     dbconn
     (string-append "replace into park_vip_info ("
                    "vip_id,carno,carno_type,name,sex,age,licence_type,licence_no"
                    ",phone,carcolor,brand,brand_sub_type,vip_type,expiry_date,"
                    "remain_money,valid_scope,delete_status,freeze_status,freeze_until,vip_discount)"
                    " values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)")
     vip_id carno carno_type name sex age licence_type licence_no
     phone carcolor brand brand_sub_type vip_type expiry_date
     remain_money valid_scope delete_status freeze_status freeze_until vip_discount))

  ;; db connect
  (define dbconn (mysql-connect #:user "ivmpuser"
                                #:database "parkdbtest"
                                #:server "192.168.1.86"
                                #:password "headway2013"))
  ;; client connect
  (define cconn (client-connect *server-ip* *server-port*))



  ;; clear table data
  (db-clear dbconn)

  (displayln "check login ...")
  (check-equal? (client-login cconn "admin" "123456")
                (hasheq 'CmdType  "LoginResponse"
                        'Result  "ERROR"
                        'ErrorInfo "Invalid Name OR Password"
                        'ErrorCode 3))

  ;; park info
  (query-exec dbconn "insert into park_info (parkcode, parkname, roadid, berth_count, remain_berth_count) values (?, ?, ?, ?, ?)"
              "parkcode1" "parkname1" "roadid1" 200 200)

  ;; user info
  (query-exec dbconn "insert into park_user_info (userid, password, username) values (?, ?, ?)"
              "admin" "123456" "admin")

  (set! cconn (client-connect *server-ip* *server-port*))

  (check-equal? (client-login cconn "admin" "123456") (hasheq 'CmdType  "LoginResponse"
                                                              'User "admin"
                                                              'Result  "OK"
                                                              'Name "admin"))

  (displayln "check shift ...")
  (check-equal? (client-shift cconn "admin" "123456" "admin1" "123456")
                (hasheq 'CmdType  "ShiftResponse"
                        'Result  "ERROR"
                        'ErrorInfo "Invalid Name OR Password"
                        'ErrorCode 3))

  (query-exec dbconn "insert into park_user_info (userid, password, username) values (?, ?, ?)"
              "admin1" "123456" "admin1")

  (check-equal? (client-shift cconn "admin" "123456" "admin1" "123456")
                (hasheq 'CmdType  "ShiftResponse"
                        'Result  "OK"
                        'User "admin1"
                        'Name "admin1"))

  (check-equal? (client-login cconn "admin" "123456")
                (hasheq 'CmdType  "LoginResponse"
                        'User "admin"
                        'Result  "OK"
                        'Name "admin"))

  (displayln "check query gateway ...")
  (check-equal? (client-query-gateway cconn)
                (hasheq 'CmdType  "QueryGatewayResponse"
                        'Result  "OK"
                        'List 'null))

  (query-exec dbconn "insert into gate_info (pointcode, pointname, point_func, parkcode, position) values (?, ?, ?, ?, ?)"
              "001" "南出入口" 3 1 0)

  (check-equal? (client-query-gateway cconn)
                (hasheq 'CmdType  "QueryGatewayResponse"
                        'Result  "OK"
                        'List (list (hasheq 'Name "南出入口"
                                            'ID "001"
                                            'Park "1"
                                            'Type 3))))

  (query-exec dbconn "insert into gate_info (pointcode, pointname, point_func, parkcode, position) values (?, ?, ?, ?, ?)"
              "002" "东入口" 1 1 0)

  (check-match (client-query-gateway cconn)
               (hash-table ((quasiquote CmdType) "QueryGatewayResponse")
                           ((quasiquote Result) "OK")
                           ((quasiquote List) (list-no-order (hash-table ((quote Name) "南出入口")
                                                                         ((quote ID) "001")
                                                                         ((quote Park) "1")
                                                                         ((quote Type) 3))
                                                             (hash-table ((quote Name) "东入口")
                                                                         ((quote ID) "002")
                                                                         ((quote Park) "1")
                                                                         ((quote Type) 1))))))

  (displayln "check query device ...")
  (check-equal? (client-query-device cconn)
                (hasheq 'CmdType  "QueryDeviceResponse"
                        'Result  "OK"
                        'List 'null))

  (query-exec
   dbconn
   "insert into equip_info (devicecode, devicename, pointcode, ip, port, videofunc) values (?, ?, ?, ?, ?, ?)"
   "0001" "入口抓拍" "001" *device1-ip* 5000 9)

  (check-match (client-query-device cconn)
               (hash-table ('CmdType  "QueryDeviceResponse")
                           ('Result  "OK")
                           ('List (list-no-order (hash-table ('Port 5000)
                                                             ('Name "入口抓拍")
                                                             ('ID "0001")
                                                             ('Type 9)
                                                             ('Gateway "001")
                                                             ('IP (? (equal-val? *device1-ip*))))))))
  (query-exec
   dbconn
   "insert into equip_info (devicecode, devicename, pointcode, ip, port, videofunc) values (?, ?, ?, ?, ?, ?)"
   "0002" "出口抓拍" "001" *device2-ip* 5000 10)

  (check-match (client-query-device cconn)
               (hash-table ((quote CmdType) "QueryDeviceResponse")
                           ((quote Result) "OK")
                           ((quote List) (list-no-order (hash-table ((quasiquote Port) 5000)
                                                                    ((quasiquote Name) "入口抓拍")
                                                                    ((quasiquote ID) "0001")
                                                                    ((quasiquote Type) 9)
                                                                    ((quasiquote Gateway) "001")
                                                                    ((quasiquote IP) (? (equal-val? *device1-ip*))))
                                                        (hash-table ((quasiquote Port) 5000)
                                                                    ((quasiquote Name) "出口抓拍")
                                                                    ((quasiquote ID) "0002")
                                                                    ((quasiquote Type) 10)
                                                                    ((quasiquote Gateway) "001")
                                                                    ((quasiquote IP) (? (equal-val? *device2-ip*))))))))

  (displayln "check operate gateway ...")
  (check-match (client-operate-gateway cconn "0001" "001" 0)
               (hash-table ((quote CmdType)  "OperateGatewayResponse")
                           ((quote Result)  "ERROR")
                           ((quote ErrorInfo) _)
                           ((quote ErrorCode) 10)))

  (check-equal? (client-operate-gateway cconn "0001" "001" 1)
                (hasheq 'CmdType  "OperateGatewayResponse"
                        'Result  "ERROR"
                        'ErrorInfo "Not Connect Gateway Device"
                        'ErrorCode 10))

  (check-equal? (client-operate-gateway cconn "0001" "001" 2)
                (hasheq 'CmdType  "OperateGatewayResponse"
                        'Result  "ERROR"
                        'ErrorInfo "Not Connect Gateway Device"
                        'ErrorCode 10))

  (define p (place p1 (dtclient-loop-forever *server-ip* *dts-port*)))

  (sleep 5)

  (check-equal? (client-operate-gateway cconn "0001" "001" 0)
                (hasheq 'CmdType  "OperateGatewayResponse"
                        'Result  "OK"))

  (displayln "check query parking vehicle ...")
  (check-equal? (client-query-parking-vehicle cconn)
                (hasheq 'CmdType  "QueryParkingVehicleResponse"
                        'Result  "OK"
                        'EnterVehicleList 'null))

  (post-record *server-ip* *server-http-port* "0001" *record1*)

  (displayln "记录1入口通知...")
  (define *record1-notify* (client-recv cconn))
  (check-enter-vehicle-notify *record1-notify* "0001" *record1*)
  (define *record1-passid* (hash-ref *record1-notify* 'PassID))

  (define-syntax (check-query-parking-vehicle stx)
    (syntax-case stx ()
      ((_ args ids records)
       (let* ((match-records (make-match-record (syntax->list #'ids)
                                                (cdr (syntax->list #'records))))
              (match-clause (if (empty? match-records)
                                ''null
                                `(,(car (syntax->list #'records)) ,@match-records))))
         #`(begin
             (pretty-print '(check-match (client-query-parking-vehicle #,@(syntax->datum #'args))
                                      (hash-table ('CmdType "QueryParkingVehicleResponse")
                                                  ('Result "OK")
                                                  ('EnterVehicleList #,match-clause))))
             (check-match (client-query-parking-vehicle #,@(syntax->datum #'args))
                               (hash-table ('CmdType "QueryParkingVehicleResponse")
                                           ('Result "OK")
                                           ('EnterVehicleList #,match-clause))))))))
  (check-query-parking-vehicle (cconn) ("0001") (list *record1*))

  (post-record *server-ip* *server-http-port* "0001" *record2*)


  (displayln "记录2入口通知...")
  (define *record2-notify* (client-recv cconn))
  (check-enter-vehicle-notify *record2-notify* "0001" *record2*)

  (define *record2-passid* (hash-ref *record2-notify* 'PassID))

  (check-query-parking-vehicle (cconn)
                               ("0001" "0001") (list-no-order *record1* *record2*))
  (check-query-parking-vehicle (cconn #:start-time "2015-08-19 09:59:59")
                               ("0001" "0001") (list-no-order *record1* *record2*))
  (check-query-parking-vehicle (cconn #:start-time "2015-08-19 10:00:00")
                               ("0001" "0001") (list-no-order *record1* *record2*))
  (check-query-parking-vehicle (cconn #:start-time "2015-08-19 10:00:15")
                               ("0001") (list *record2*))
  (check-query-parking-vehicle (cconn #:start-time "2015-08-19 10:00:30")
                               ("0001") (list *record2*))
  (check-query-parking-vehicle (cconn #:start-time "2015-08-19 10:01:00")
                               () (list))
  (check-query-parking-vehicle (cconn #:start-time "2015-08-20 10:00:00")
                               () (list))
  (check-query-parking-vehicle (cconn #:end-time "2015-08-19 10:01:00")
                               ("0001" "0001") (list-no-order *record1* *record2*))
  (check-query-parking-vehicle (cconn #:end-time "2015-08-19 10:00:30")
                               ("0001" "0001") (list-no-order *record1* *record2*))
  (check-query-parking-vehicle (cconn #:end-time "2015-08-19 10:00:15")
                               ("0001") (list *record1*))
  (check-query-parking-vehicle (cconn #:end-time "2015-08-19 10:00:00")
                               ("0001") (list *record1*))
  (check-query-parking-vehicle (cconn #:end-time "2015-08-19 09:59:59")
                               () (list))
  (check-query-parking-vehicle (cconn #:end-time "2015-08-18 10:00:30")
                               () (list))
  (check-query-parking-vehicle (cconn #:vehicle-color "A")
                               ("0001") (list *record1*))
  (check-query-parking-vehicle (cconn #:vehicle-color "B")
                               ("0001") (list *record2*))
  (check-query-parking-vehicle (cconn #:vehicle-color "C")
                               () (list))
  (check-query-parking-vehicle (cconn #:plate-number "鲁")
                               () (list))
  ;; (check-query-parking-vehicle (cconn #:plate-number "鲁")
  ;;                              ("0001" "0001") (*record1* *record2*))
  (check-query-parking-vehicle (cconn #:plate-number "鲁 B88888")
                               ("0001") (list *record1*))
  (check-query-parking-vehicle (cconn #:plate-number "鲁 C66666")
                               ("0001") (list *record2*))
  (check-query-parking-vehicle (cconn #:plate-number "鲁 C66566")
                               ("0001") (list *record2*))
  (check-query-parking-vehicle (cconn #:plate-number "鲁 C66556")
                               ("0001") (list *record2*))
  (check-query-parking-vehicle (cconn #:plate-number "京 C66666")
                               ("0001") (list *record2*))
  (check-query-parking-vehicle (cconn #:plate-number "京 B66666")
                               ("0001") (list *record2*))
  (check-query-parking-vehicle (cconn #:plate-number "鲁 C66555")
                               () (list))
  (check-query-parking-vehicle (cconn #:plate-number "C")
                               () (list))
  (check-query-parking-vehicle (cconn #:plate-number "B")
                               () (list))
  (check-query-parking-vehicle (cconn #:plate-number "B8")
                               () (list))
  ;; ;; (check-query-parking-vehicle (cconn #:parking-type 5)
  ;; ;;                              () )
  ;; ;; (check-query-parking-vehicle (cconn #:parking-type 1)
  ;; ;;                              () ())
  (check-query-parking-vehicle (cconn #:start-time "2015-08-19 10:00:01" #:end-time "2015-08-19 10:00:29")
                               () (list))
  (check-query-parking-vehicle (cconn #:start-time "2015-08-19 10:00:00" #:plate-number "C")
                               () (list))        ;
  (check-query-parking-vehicle (cconn
                                #:start-time "2015-08-19 10:00:00"
                                #:plate-number "C"
                                #:vehicle-color "A")
                               () (list))
  ;; (check-query-parking-vehicle (cconn
  ;;                               #:start-time "2015-08-19 10:00:00"
  ;;                               #:plate-number "C"
  ;;                               #:vehicle-color "A")
  ;;                              () (list))
  (check-match (client-revise-plate cconn *record1-passid* "鲁 A11111")
               (hash-table ((quote CmdType) "RevisePlateResponse")
                           ((quote Result) "OK")))
  (check-query-parking-vehicle (cconn)
                               ("0001" "0001")
                               (list-no-order (hash-set *record1* 'PlateInfo1 "鲁 A11111") *record2*))

  (client-revise-plate cconn *record1-passid* (hash-ref *record1* 'PlateInfo1))

  (check-match (client-revise-plate cconn "12341234" "鲁 A22222")
               (hash-table ((quote CmdType) "RevisePlateResponse")
                           ((quote Result) "OK")))

  ;; 计费规则
  (db-insert-rule dbconn)

  ;; record2 出库
  (define *record2-exit* (hash-set* *record2* 'direction 10 'PassTime "2015-08-19 13:00:30"))
  (post-record *server-ip* *server-http-port* "0001" *record2-exit*)

  (displayln "记录2出口通知...")
  (define *record2-exit-notify* (client-recv cconn))

  (check-exit-vehicle-notify *record2-exit-notify* "0001" *record2-exit* *record2* 9 0 0)

  (define *record2-exit-passid* (hash-ref (hash-ref *record2-exit-notify* 'ExitVehicle) 'PassID))
  (define *record2-exit-charge-id* (hash-ref *record2-exit-notify* 'ChargeID))

  ;; (check-query-parking-vehicle (cconn) ("0001") (list *record1*))

  (define-simple-check (check-charge cconn enter-passid exit-passid parking-type total-charge require-charge)
    (match (client-match-plate cconn enter-passid exit-passid)
      ((hash-table ((quote Bill) (hash-table ((quote BeforeDiscountedFee) _)
                                             ((quote Discount) _)
                                             ((quote Duration) _)
                                             ((quote EnterTime) _)
                                             ((quote ExitTime) _)
                                             ((quote FreeDuration) _)
                                             ((quote List) _)
                                             ((quote TotalFee) _)))
                   ((quote CmdType) "MatchPlateResponse")
                   ((quote Result) "OK")
                   ((quote ChargeID) _)
                   ((quote ParkingType) pktype)
                   ((quote TotalCharge) tcharge)
                   ((quote RequireCharge) rcharge))
       (and (equal? pktype parking-type)
            (equal? tcharge total-charge)
            (equal? rcharge require-charge)))
      ((hash-table ((quote Bill) (hash-table ((quote BeforeDiscountedFee) _)
                                             ((quote Discount) _)
                                             ((quote Duration) _)
                                             ((quote EnterTime) _)
                                             ((quote ExitTime) _)
                                             ((quote FreeDuration) _)
                                             ((quote TotalFee) _)))
                   ((quote CmdType) "MatchPlateResponse")
                   ((quote Result) "OK")
                   ((quote ChargeID) _)
                   ((quote ParkingType) pktype)
                   ((quote TotalCharge) tcharge)
                   ((quote RequireCharge) rcharge))
       (and (equal? pktype parking-type)
            (equal? tcharge total-charge)
            (equal? rcharge require-charge)))
      ((hash-table ((quote Bill) (hash-table ((quote BeforeDiscountedFee) _)
                                             ((quote Discount) _)
                                             ((quote Duration) _)
                                             ((quote EnterTime) _)
                                             ((quote ExitTime) _)
                                             ((quote FeeExemption) _)
                                             ((quote FreeDuration) _)
                                             ((quote List) _)
                                             ((quote TotalFee) _)))
                   ((quote CmdType) "MatchPlateResponse")
                   ((quote Result) "OK")
                   ((quote ChargeID) _)
                   ((quote ParkingType) pktype)
                   ((quote TotalCharge) tcharge)
                   ((quote RequireCharge) rcharge))
       (and (equal? pktype parking-type)
            (equal? tcharge total-charge)
            (equal? rcharge require-charge)))))

  (define-syntax (with-rule stx)
    (syntax-case stx ()
      ((_ dbconn rules . expr)
       #`(begin
           (db-clear-table dbconn "park_rule_config")
           #,@(map (lambda (rule) #`(db-insert-rule dbconn #,@(syntax->datum rule))) (syntax->list #'rules))
           #,@(syntax->list #'expr)
           (db-clear-table dbconn "park_rule_config")))))

  (with-rule dbconn (())
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 0 0))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 5))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 5 5))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 6 6))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig -1))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 -1 -1))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:free_time_sections "11:00-12:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 4 4))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:free_time_sections "11:00-12:01"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 4 4))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:free_time_sections "11:00-11:59"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 5 5))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:day_begin "14:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 0 0))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:night_begin "10:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 0 0))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:night_begin "11:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 2 2))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:night_begin "12:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 4 4))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:day_begin "12:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 2 2))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:day_begin "12:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 2 2))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:free_time_length 30))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 5 5))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:free_time_length 60))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 4 4))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:free_time_length 90))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 3 3))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:free_time_length 120))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 2 2))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:free_time_length 150))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 1 1))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:free_time_length 180))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 0 0))
  (with-rule dbconn ((#:nightfeeratebig 2 #:maxfeebig 10 #:night_begin "10:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 10 10))
  (with-rule dbconn ((#:nightfeeratebig 2 #:maxfeebig 20 #:night_begin "10:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 12 12))
  (with-rule dbconn ((#:nightfeeratebig 2 #:maxfeebig 20 #:night_begin "11:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 8 8))
  (with-rule dbconn ((#:nightfeeratebig 2 #:maxfeebig 20 #:night_begin "11:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 8 8))
  (with-rule dbconn ((#:nightfeeratebig 2 #:maxfeebig 20 #:night_begin "11:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 8 8))
  (with-rule dbconn ((#:nightfeeratebig 2 #:nightfeespanbig 60 #:maxfeebig 20 #:night_begin "10:00"))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 6 6))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:park_position 1))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 0 0))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:park_position 1)
                     (#:nid "rule2" #:name "rule2" #:dayfeeratebig 2 #:maxfeebig 10 #:week_day 3))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 10 10))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10 #:park_position 1 #:invalid_flag 0))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 0 0))
  (with-rule dbconn ((#:dayfeeratebig 1 #:maxfeebig 10)
                     (#:nid "rule2" #:name "rule2" #:rule_priority 4 #:dayfeeratebig 2 #:maxfeebig 10 #:invalid_flag 0))
             (check-charge cconn *record2-passid* *record2-exit-passid* 9 6 6))


  (displayln "记录3入口通知...")
  (define *record3* (hash-set* *record2* 'PlateInfo1 "鲁 B22222" 'PlateType 2))
  (post-record *server-ip* *server-http-port* "0001" *record3*)
  (define *record3-notify* (client-recv cconn))
  (check-enter-vehicle-notify *record3-notify* "0001" *record3*)
  (define *record3-passid* (hash-ref *record3-notify* 'PassID))

  ;; (check-query-parking-vehicle (cconn) ("0001" "0001") (list-no-order *record1* *record3*))

  (displayln "记录3出口通知...")
  (define *record3-exit* (hash-set* *record3* 'direction 10 'PassTime "2015-08-19 13:00:30"))
  (post-record *server-ip* *server-http-port* "0001" *record3-exit*)
  (define *record3-exit-notify* (client-recv cconn))
  (check-exit-vehicle-notify *record3-exit-notify* "0001" *record3-exit* *record3* 9 0 0)
  (define *record3-exit-passid* (hash-ref (hash-ref *record3-exit-notify* 'ExitVehicle) 'PassID))

  ;; (check-query-parking-vehicle (cconn) ("0001") (list *record1*))

  (with-rule dbconn (())
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 5 5))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 6 6))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee -1))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 -1 -1))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-12:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-12:01"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-11:59"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 5 5))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "14:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "10:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "11:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "12:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "12:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "12:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 30))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 5 5))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 60))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 90))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 3 3))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 120))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 150))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 1 1))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 180))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
  (with-rule dbconn ((#:nightfeerate 2 #:maxfee 10 #:night_begin "10:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 10 10))
  (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "10:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 12 12))
  (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 8 8))
  (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 8 8))
  (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 8 8))
  (with-rule dbconn ((#:nightfeerate 2 #:nightfeespan 60 #:maxfee 20 #:night_begin "10:00"))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 6 6))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1)
                     (#:nid "rule2" #:name "rule2" #:dayfeerate 2 #:maxfee 10 #:week_day 3))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 10 10))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1 #:invalid_flag 0))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10)
                     (#:nid "rule2" #:name "rule2" #:rule_priority 4 #:dayfeerate 2 #:maxfee 10 #:invalid_flag 0))
             (check-charge cconn *record3-passid* *record3-exit-passid* 9 6 6))

  (define-syntax (with-vip stx)
    (syntax-case stx ()
      ((_ dbconn vip . expr)
       #`(begin
           (db-clear-table dbconn "park_vip_info")
           (db-insert-vip dbconn #,@(syntax->datum #'vip))
           #,@(syntax->list #'expr)
           (db-clear-table dbconn "park_vip_info")))))                         ;

  (displayln "check vip...")
  (with-vip dbconn (#:carno "鲁 B22222")
            (with-rule dbconn (())
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee -1))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-12:01"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-11:59"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "14:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 30))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 60))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 90))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 120))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 150))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 180))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 10 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:nightfeespan 60 #:maxfee 20 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1)
                               (#:nid "rule2" #:name "rule2" #:dayfeerate 2 #:maxfee 10 #:week_day 3))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1 #:invalid_flag 0))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10)
                               (#:nid "rule2" #:name "rule2" #:rule_priority 4 #:dayfeerate 2 #:maxfee 10 #:invalid_flag 0))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 1 0 0)))

  (with-vip dbconn (#:carno "鲁 B22222" #:vip_type 3 #:expiry_date "2015-08-20 10:00:00")
            (with-rule dbconn (())
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee -1))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-12:01"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-11:59"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "14:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 30))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 60))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 90))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 120))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 150))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 180))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 10 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:nightfeespan 60 #:maxfee 20 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1)
                               (#:nid "rule2" #:name "rule2" #:dayfeerate 2 #:maxfee 10 #:week_day 3))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1 #:invalid_flag 0))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10)
                               (#:nid "rule2" #:name "rule2" #:rule_priority 4 #:dayfeerate 2 #:maxfee 10 #:invalid_flag 0))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 3 0 0)))

  (with-vip dbconn (#:carno "鲁 B22222" #:vip_type 3 #:expiry_date "2015-08-19 12:00:00")
            (with-rule dbconn (())
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee -1))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 -1 -1))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-12:01"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-11:59"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "14:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 30))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 1 1))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 60))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 90))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 120))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 150))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 180))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 10 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
            (with-rule dbconn ((#:nightfeerate 2 #:nightfeespan 60 #:maxfee 20 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1)
                               (#:nid "rule2" #:name "rule2" #:dayfeerate 2 #:maxfee 10 #:week_day 3))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1 #:invalid_flag 0))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10)
                               (#:nid "rule2" #:name "rule2" #:rule_priority 4 #:dayfeerate 2 #:maxfee 10 #:invalid_flag 0))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2)))

  (with-vip dbconn (#:carno "鲁 B22222" #:vip_type 2 #:remain_money 0)
            (with-rule dbconn (())
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 5 5))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 6 6))
            ;; (with-rule dbconn ((#:dayfeerate 1 #:maxfee -1))
            ;;            (check-charge cconn *record3-passid* *record3-exit-passid* 2 -1 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-12:01"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_sections "11:00-11:59"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 5 5))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "14:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:night_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:day_begin "12:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 30))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 5 5))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 60))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 90))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 3 3))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 120))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 2 2))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 150))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 1 1))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:free_time_length 180))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 0 0))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 10 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 10 10))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 12 12))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 8 8))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 8 8))
            (with-rule dbconn ((#:nightfeerate 2 #:maxfee 20 #:night_begin "11:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 8 8))
            (with-rule dbconn ((#:nightfeerate 2 #:nightfeespan 60 #:maxfee 20 #:night_begin "10:00"))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 6 6))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1)
                               (#:nid "rule2" #:name "rule2" #:dayfeerate 2 #:maxfee 10 #:week_day 3))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 10 10))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10 #:park_position 1 #:invalid_flag 0))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 0 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 10)
                               (#:nid "rule2" #:name "rule2" #:rule_priority 4 #:dayfeerate 2 #:maxfee 10 #:invalid_flag 0))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 6 6)))

  (displayln "测试储值会员...")
  (with-vip dbconn (#:carno "鲁 B22222" #:vip_type 2 #:remain_money 20)
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 5 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 5 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 5 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 5 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 5 5)))

  (displayln "测试会员折扣...")
  (with-vip dbconn (#:carno "鲁 B22222" #:vip_type 2 #:remain_money 20 #:vip_discount 0.8)
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 4 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 4 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 4 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 4 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 2 4 0))
            (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
                       (check-charge cconn *record3-passid* *record3-exit-passid* 9 4 4)))


  (define-simple-check (check-checkout cconn enter-passid exit-passid
                                       actual-charge pay-type
                                       exit-type exit-comment
                                       result error-info)
    (let* ((match-result (client-match-plate cconn enter-passid exit-passid))
           (checkout-result (client-checkout cconn enter-passid exit-passid
                                             (hash-ref match-result 'RequireCharge)
                                             actual-charge
                                             pay-type
                                             (hash-ref match-result 'ParkingType)
                                             exit-type exit-comment
                                             (hash-ref match-result 'ChargeID)
                                             (hash-ref match-result 'Bill))))
      (or (equal? checkout-result
                  (hasheq 'CmdType "CheckoutResponse"
                          'Result result))
          (equal? checkout-result
                  (hasheq 'CmdType "CheckoutResponse"
                          'Result result
                          'ErrorInfo error-info)))))

  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
             (check-checkout cconn *record3-passid* *record3-exit-passid*
                             5 1
                             2 "test exit comment"
                             "OK" ""))

  (displayln "添加出入口车牌不一致的情况...")
  (displayln "记录4入口通知...")
  (define *record4* (hash-set* *record2* 'PlateInfo1 "鲁 B33333" 'PlateType 2))
  (post-record *server-ip* *server-http-port* "0001" *record4*)
  (define *record4-notify* (client-recv cconn))
  (check-enter-vehicle-notify *record4-notify* "0001" *record4*)
  (define *record4-passid* (hash-ref *record4-notify* 'PassID))

  ;; (check-query-parking-vehicle (cconn) ("0001" "0001") (list-no-order *record1* *record4*))

  (displayln "记录4出口通知...")
  (define *record4-exit* (hash-set* *record4* 'PlateInfo1 "鲁 B44444" 'direction 10 'PassTime "2015-08-19 13:00:30"))
  (post-record *server-ip* *server-http-port* "0001" *record4-exit*)
  (define *record4-exit-notify* (client-recv cconn))
  (check-exit-vehicle-notify *record4-exit-notify* "0001" *record4-exit*)
  (define *record4-exit-passid* (hash-ref (hash-ref *record4-exit-notify* 'ExitVehicle) 'PassID))

  ;; (check-query-parking-vehicle (cconn) ("0001" "0001") (list-no-order *record1* *record4*))

  (check-match (client-revise-plate cconn *record4-passid* "鲁 B44444")
               (hash-table ((quote CmdType) "RevisePlateResponse")
                           ((quote Result) "OK")))

  (with-rule dbconn ((#:dayfeerate 1 #:maxfee 5))
             (check-checkout cconn *record4-passid* *record4-exit-passid*
                             5 1
                             2 "test exit comment"
                             "OK" ""))

  (check-equal? (client-query-parking-number cconn)
                (hasheq 'CmdType "QueryParkingNumberResponse"
                        'Result "OK"
                        'TotalParkingNumber 200
                        'RemainParkingNumber 199))

  (check-equal? (client-revise-remain-parking-number cconn 100)
                (hasheq 'CmdType "ReviseRemainParkingNumberResponse"
                        'Result "OK"))

  (check-equal? (client-query-parking-number cconn)
                (hasheq 'CmdType "QueryParkingNumberResponse"
                        'Result "OK"
                        'TotalParkingNumber 200
                        'RemainParkingNumber 100))

  (place-kill p)
  (disconnect dbconn)))





(define (test-post-record-loop)
  (let loop ()
    (thread (lambda ()
              (post-record *server-ip* *server-http-port* "0001" *record2*)
              (define *record2-exit* (hash-set* *record2* 'direction 10 'PassTime "2015-08-19 13:00:30"))
              (post-record *server-ip* *server-http-port* "0001" *record2-exit*)))
    (loop)))

(define (test-time)
  ;; client connect
  (define cconn (client-connect *server-ip* *server-port*))
  (client-login cconn "admin" "123456")
  (time (post-record *server-ip* *server-http-port* "0001" *record2*)
        (displayln (client-recv cconn)))

  (define *record2-exit* (hash-set* *record2* 'direction 10 'PassTime "2015-08-19 13:00:30"))
  (time (post-record *server-ip* *server-http-port* "0001" *record2-exit*)
        (displayln (client-recv cconn))))
