# 4wd-car
4輪傳動小車

# 4wd_car_esp32_blynk.ino
使用blynk APP作為控制端來遙控小車
小車使用esp32作為控制核心，經由WIFI連接AP後，再連上blynk server接收訊息
小車離開WIFI範圍會失控，故要做好失去WIFI連線的異常處理機制

