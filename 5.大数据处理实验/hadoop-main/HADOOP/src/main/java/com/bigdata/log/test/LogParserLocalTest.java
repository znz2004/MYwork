package com.bigdata.log.test;

import com.bigdata.log.utils.LogParser;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Map;

/**
 * 本地测试LogParser，从文件读取日志
 */
public class LogParserLocalTest {
    public static void main(String[] args) {
        String sampleFile = "dataset/access_log_sample.txt";
        int totalLines = 0;
        int successfullyParsed = 0;
        int failedToParse = 0;
        
        try (BufferedReader reader = new BufferedReader(new FileReader(sampleFile))) {
            String line;
            while ((line = reader.readLine()) != null) {
                totalLines++;
                
                // 解析日志行
                Map<String, String> parsedLog = LogParser.parseLogLine(line);
                
                if (parsedLog.containsKey("error")) {
                    failedToParse++;
                    if (failedToParse <= 5) { // 只显示前5个错误
                        System.out.println("解析失败: " + line);
                        System.out.println("错误: " + parsedLog.get("error"));
                        System.out.println();
                    }
                } else {
                    successfullyParsed++;
                    
                    // 打印前5个成功解析的样本
                    if (successfullyParsed <= 5) {
                        System.out.println("解析成功样本 #" + successfullyParsed + ":");
                        System.out.println("原始日志: " + line);
                        System.out.println("----- 解析结果 -----");
                        for (Map.Entry<String, String> entry : parsedLog.entrySet()) {
                            System.out.println(entry.getKey() + ": " + entry.getValue());
                        }
                        System.out.println("\n");
                    }
                }
            }
            
            // 打印统计信息
            System.out.println("解析统计:");
            System.out.println("总日志行数: " + totalLines);
            System.out.println("成功解析: " + successfullyParsed + " (" + (successfullyParsed * 100.0 / totalLines) + "%)");
            System.out.println("解析失败: " + failedToParse + " (" + (failedToParse * 100.0 / totalLines) + "%)");
            
        } catch (IOException e) {
            System.err.println("读取文件时出错: " + e.getMessage());
            e.printStackTrace();
        }
    }
} 