package com.bigdata.log.test;

import com.bigdata.log.utils.LogParser;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.TreeMap;

/**
 * 本地测试每小时访问量统计，不使用Hadoop
 */
public class HourlyVisitLocalTest {
    public static void main(String[] args) {
        String sampleFile = "dataset/access_log_sample.txt";
        int totalLines = 0;
        int processedLines = 0;
        
        // 用于统计每小时访问量的Map
        Map<String, Integer> hourlyVisitMap = new HashMap<>();
        
        try (BufferedReader reader = new BufferedReader(new FileReader(sampleFile))) {
            String line;
            while ((line = reader.readLine()) != null) {
                totalLines++;
                
                // 解析日志行
                Map<String, String> parsedLog = LogParser.parseLogLine(line);
                
                // 检查解析是否成功
                if (!parsedLog.containsKey("error")) {
                    processedLines++;
                    
                    // 提取date_hour字段，作为小时级访问量统计的键
                    String dateHour = parsedLog.get("date_hour");
                    if (dateHour != null && !dateHour.equals("Unknown")) {
                        // 更新计数
                        hourlyVisitMap.put(dateHour, hourlyVisitMap.getOrDefault(dateHour, 0) + 1);
                    }
                }
            }
            
            // 将结果按键排序（按时间顺序）
            Map<String, Integer> sortedResults = new TreeMap<>(hourlyVisitMap);
            
            // 打印统计结果
            System.out.println("每小时访问量统计结果:");
            System.out.println("------------------------");
            System.out.println("时间戳\t\t访问量");
            System.out.println("------------------------");
            for (Map.Entry<String, Integer> entry : sortedResults.entrySet()) {
                System.out.println(entry.getKey() + "\t" + entry.getValue());
            }
            System.out.println("------------------------");
            
            // 打印统计信息
            System.out.println("\n处理统计:");
            System.out.println("总日志行数: " + totalLines);
            System.out.println("成功处理的行数: " + processedLines);
            System.out.println("不同小时数: " + sortedResults.size());
            
        } catch (IOException e) {
            System.err.println("读取文件时出错: " + e.getMessage());
            e.printStackTrace();
        }
    }
} 