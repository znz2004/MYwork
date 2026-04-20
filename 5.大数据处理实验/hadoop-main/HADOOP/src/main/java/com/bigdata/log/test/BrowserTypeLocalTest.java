package com.bigdata.log.test;

import com.bigdata.log.utils.LogParser;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.stream.Collectors;

/**
 * 本地测试浏览器类型统计，不使用Hadoop
 */
public class BrowserTypeLocalTest {
    public static void main(String[] args) {
        String sampleFile = "dataset/access_log_sample.txt";
        int totalLines = 0;
        int processedLines = 0;
        
        // 用于统计浏览器类型的Map
        Map<String, Integer> browserTypeMap = new HashMap<>();
        
        try (BufferedReader reader = new BufferedReader(new FileReader(sampleFile))) {
            String line;
            while ((line = reader.readLine()) != null) {
                totalLines++;
                
                // 解析日志行
                Map<String, String> parsedLog = LogParser.parseLogLine(line);
                
                // 检查解析是否成功
                if (!parsedLog.containsKey("error")) {
                    processedLines++;
                    
                    // 提取浏览器类型
                    String browserType = parsedLog.get("browser_type");
                    if (browserType != null) {
                        // 更新计数
                        browserTypeMap.put(browserType, browserTypeMap.getOrDefault(browserType, 0) + 1);
                    }
                }
            }
            
            // 按照访问量排序（降序）
            Map<String, Integer> sortedResults = browserTypeMap.entrySet()
                .stream()
                .sorted(Map.Entry.<String, Integer>comparingByValue().reversed())
                .collect(Collectors.toMap(
                    Map.Entry::getKey, 
                    Map.Entry::getValue, 
                    (e1, e2) -> e1, 
                    LinkedHashMap::new
                ));
            
            // 打印统计结果
            System.out.println("浏览器类型统计结果:");
            System.out.println("------------------------");
            System.out.println("浏览器类型\t\t访问量\t百分比");
            System.out.println("------------------------");
            for (Map.Entry<String, Integer> entry : sortedResults.entrySet()) {
                double percentage = (entry.getValue() * 100.0) / processedLines;
                System.out.printf("%s\t\t%d\t%.2f%%\n", entry.getKey(), entry.getValue(), percentage);
            }
            System.out.println("------------------------");
            
            // 打印统计信息
            System.out.println("\n处理统计:");
            System.out.println("总日志行数: " + totalLines);
            System.out.println("成功处理的行数: " + processedLines);
            System.out.println("不同浏览器类型数: " + sortedResults.size());
            
        } catch (IOException e) {
            System.err.println("读取文件时出错: " + e.getMessage());
            e.printStackTrace();
        }
    }
} 