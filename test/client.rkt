#lang racket

(require net/rfc6455
         net/url
         json)

(provide client-connect
         client-recv
         client-send-recv
         client-login
         client-shift
         client-query-gateway
         client-query-device
         client-operate-gateway
         client-query-parking-vehicle
         client-revise-plate
         client-match-plate
         client-checkout
         client-query-parking-number
         client-revise-remain-parking-number)

(define (client-connect ip port)
  (ws-connect (string->url (format "ws://~a:~a/" ip port))))

(define (client-send-recv conn val)
  (ws-send! conn (jsexpr->string val))
  (string->jsexpr (ws-recv conn)))

(define (client-recv conn)
  (string->jsexpr (ws-recv conn)))

(define (client-login conn user pass)
  (client-send-recv conn (hash 'CmdType "Login"
                               'User user
                               'Password pass)))
(define (client-shift conn user pass user1 pass1)
  (client-send-recv conn (hash 'CmdType "Shift"
                               'CurrentUser user
                               'CurrentPassword pass
                               'NextUser user1
                               'NextPassword pass1)))

(define (client-query-gateway conn)
  (client-send-recv conn (hash 'CmdType "QueryGateway")))

(define (client-query-device conn)
  (client-send-recv conn (hash 'CmdType "QueryDevice")))

;; @act 0关 1开 2停
(define (client-operate-gateway conn devid passid act) 
  (client-send-recv conn (hash 'CmdType "OperateGateway"
                               'DeviceID devid
                               'PassID passid
                               'ActionType act)))

(define (client-query-parking-vehicle conn
                                      #:start-time [start-time #f]
                                      #:end-time [end-time #f]
                                      #:vehicle-color [vehicle-color #f]
                                      #:plate-number [plate-number #f]
                                      #:parking-type [parking-type #f])
  (let ((kv-list (append (if start-time
                             (list 'StartTime start-time)
                             null)
                         (if end-time
                             (list 'EndTime end-time)
                             null)
                         (if vehicle-color
                             (list 'VehicleColor vehicle-color)
                             null)
                         (if plate-number
                             (list 'PlateNumber plate-number)
                             null)
                         (if parking-type
                             (list 'ParkingType parking-type)
                             null))))
    (client-send-recv conn (apply hasheq 'CmdType "QueryParkingVehicle"
                                  kv-list))))

(define (client-revise-plate conn passid plate-number)
  (client-send-recv conn (hasheq 'CmdType "RevisePlate"
                                 'PassID passid
                                 'PlateNumber plate-number)))

(define (client-match-plate conn enter-id exit-id)
  (client-send-recv conn (hasheq 'CmdType "MatchPlate"
                                 'EnterID enter-id
                                 'ExitID exit-id)))

(define (client-checkout conn enter-id exit-id charge actual-charge pay-type parking-type exit-type exit-comment charge-id bill)
  (client-send-recv conn (hasheq 'CmdType "Checkout"
                                 'EnterID enter-id
                                 'ExitID exit-id
                                 'Charge charge
                                 'ActualCharge actual-charge
                                 'PayType pay-type
                                 'ParkingType parking-type
                                 'ExitType exit-type
                                 'ExitComment exit-comment
                                 'ChargeID charge-id
                                 'Bill bill)))

(define (client-query-parking-number conn)
  (client-send-recv conn (hasheq 'CmdType "QueryParkingNumber")))

(define (client-revise-remain-parking-number conn remain-parking-number)
  (client-send-recv conn (hasheq 'CmdType "ReviseRemainParkingNumber"
                                 'RemainParkingNumber remain-parking-number)))
(define (client-record-esc conn leave_nid match_nids cacl_fee time)
  (client-send-recv conn (hasheq 'CmdType "EscRecord"
                                 'LeaveNid leave_nid
                                 'MatchNids match_nids
                                 'CaclFee cacl_fee
                                 'Time time)))

(define (test-record-esc)
  (define conn (client-connect "192.168.1.94" 9000))
  (client-login conn "admin" "123456")
  (client-record-esc conn "leave_nid1" '() 10 "2015-08-19 13:00:30")
  (client-record-esc conn "leave_nid1" '("match_nid1") 10 "2015-08-19 13:00:30")
  (client-record-esc conn "leave_nid2" '("match_nid2" "match_nid3") 0 "2015-08-19 13:00:31"))
