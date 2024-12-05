## 目的說明
在2016年，因應稽核對電腦機房需要溫溼度監控，市面上販售的設備大部分為顯示，資料需要人為盤點，製作可上傳的溫溼度的裝置
下圖為製作時間
https://www.facebook.com/photo.php?fbid=10205967374581965&set=pb.1788131020.-2207520000&type=3
## 架構說明
1.使用arduino nano 、 ENC28J60乙太網路接口、DHT21*2 、DS3231 RTC、LCD1602液晶顯示IIC/I2C
2.需要撰寫ENC28J60乙太網路接口對應程式。
3.因為nano的記憶體不大，無法寫太多程式，在使用指令要特別注意與記憶體回收
4.在製作過程，有發生程式過大，導致nano loader發生異常!使用MEGA2560做一個loader燒錄器將loader寫回nano
https://github.com/godspeed890603/httpPost4/blob/master/device/465604082_10221891034263505_8094584157316580134_n.jpg


## 成果
https://github.com/godspeed890603/httpPost4/blob/master/device/%E6%88%90%E5%93%81.jpg
https://github.com/godspeed890603/httpPost4/blob/master/device/72816280_10212365114721470_6446866131725582336_n.jpg
