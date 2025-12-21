-- Просмотр всех устройств
SELECT * FROM Devices ORDER BY device_id;

-- Просмотр всех типов работ
SELECT * FROM Service_Types ORDER BY service_id;

-- Просмотр истории обслуживания
SELECT * FROM Service_History ORDER BY service_date DESC;

-- Запрос для просмотра просроченного обслуживания
SELECT 
    d.name as устройство,
    d.model as модель,
    st.name as тип_обслуживания,
    sh.service_date as последнее_обслуживание,
    sh.next_due_date as срок_следующего,
    (CURRENT_DATE - sh.next_due_date) as дней_просрочено
FROM Service_History sh
JOIN Devices d ON sh.device_id = d.device_id
JOIN Service_Types st ON sh.service_id = st.service_id
WHERE sh.next_due_date < CURRENT_DATE
AND d.status = 'active'
ORDER BY sh.next_due_date;

-- Статистика по затратам на обслуживание
SELECT 
    d.name,
    COUNT(sh.record_id) as количество_обслуживаний,
    SUM(sh.cost) as общая_стоимость,
    MAX(sh.service_date) as последнее_обслуживание
FROM Devices d
LEFT JOIN Service_History sh ON d.device_id = sh.device_id
GROUP BY d.name
ORDER BY общая_стоимость DESC NULLS LAST;

-- Ближайшие плановые обслуживания
SELECT 
    d.name,
    st.name as тип_работы,
    sh.next_due_date as следующее_обслуживание,
    (sh.next_due_date - CURRENT_DATE) as дней_до_обслуживания
FROM Service_History sh
JOIN Devices d ON sh.device_id = d.device_id
JOIN Service_Types st ON sh.service_id = st.service_id
WHERE sh.next_due_date >= CURRENT_DATE
AND sh.next_due_date <= CURRENT_DATE + INTERVAL '30 days'
AND d.status = 'active'
ORDER BY sh.next_due_date;