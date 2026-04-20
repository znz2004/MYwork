package com.bigdata.log;

import com.bigdata.log.utils.LogParser;
import java.util.Map;

/**
 * LogParser测试类
 */
public class LogParserTest {
    public static void main(String[] args) {
        // 测试用的日志样本
        String[] logSamples = {
            "221.179.161.41 - - [18/Sep/2013:19:08:30] \"GET /wp-content/themes/silesia/images/ico-twitter.png HTTP/1.1\" [200] 138501 \"-\" \"DNSPod-Monitor/1.0\"",
            "118.250.160.214 - - [18/Sep/2013:00:12:32] \"GET /2013/page/12/ HTTP/1.1\" [200] 20 \"http://www.angularjs.cn/A0eQ\" \"Mozilla/5.0 (Linux; U; Android 4.0.4; zh-cn; MI 1S Build/IMM76D) AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/534.30 V1_AND_SQ_4.2.1_3_YYB_D\"",
            "163.177.71.12 - - [18/Sep/2013:15:37:04] \"GET /tag/pptp/ HTTP/1.1\" [200] 0 \"-\" \"Mozilla/5.0 (compatible; SiteExplorer/1.0b; +http://siteexplorer.info/)\""
        };
        
        // 测试每个样本
        for (int i = 0; i < logSamples.length; i++) {
            System.out.println("测试样本 #" + (i + 1) + ":");
            System.out.println("原始日志: " + logSamples[i]);
            
            Map<String, String> parsedLog = LogParser.parseLogLine(logSamples[i]);
            
            if (parsedLog.containsKey("error")) {
                System.out.println("解析失败: " + parsedLog.get("error"));
            } else {
                System.out.println("解析成功!");
                System.out.println("----- 解析结果 -----");
                for (Map.Entry<String, String> entry : parsedLog.entrySet()) {
                    System.out.println(entry.getKey() + ": " + entry.getValue());
                }
            }
            System.out.println("\n");
        }
    }
} 