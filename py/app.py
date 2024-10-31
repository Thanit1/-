import pandas as pd
from datetime import datetime

# อ่านไฟล์ Excel
df = pd.read_excel('1.xlsx')

# แปลงคอลัมน์วันเวลาให้เป็น datetime
df['วันเวลา'] = pd.to_datetime(df['วันเวลา'])

# สร้างคอลัมน์วันที่
df['วันที่'] = df['วันเวลา'].dt.date

# จัดกลุ่มข้อมูลตามวันที่และหมายเลขช่อง แล้วรวมค่าหน่วยการใช้ไฟฟ้า
result = df.groupby(['วันที่', 'หมายเลขช่อง'])['หน่วยการใช้ไฟฟ้า (kWh)'].sum().reset_index()

# สร้าง pivot table เพื่อแยกหมายเลขช่องเป็นคอลัมน์
pivot_result = result.pivot(index='วันที่', columns='หมายเลขช่อง', values='หน่วยการใช้ไฟฟ้า (kWh)')

# เปลี่ยนชื่อคอลัมน์
pivot_result.columns = [f'ช่อง {col}' for col in pivot_result.columns]

# รีเซ็ตดัชนีและรีเซ็ตชื่อคอลัมน์วันที่
pivot_result = pivot_result.reset_index().rename(columns={'วันที่': 'วันที่'})

# บันทึกผลลัพธ์เป็นไฟล์ Excel ใหม่
pivot_result.to_excel('output.xlsx', index=False)

print("สร้างไฟล์ output.xlsx เสร็จสมบูรณ์")