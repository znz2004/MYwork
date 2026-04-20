package com.bigdata.log.utils;

import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * 日志解析工具类
 */
public class LogParser {
    // 修改后的正则表达式，适应实际日志格式
    private static final Pattern LOG_PATTERN = Pattern.compile(
            "(\\S+) (\\S+) (\\S+) \\[(\\S+):(\\S+):(\\S+)\\] \"(\\S+) (\\S+) (\\S+)\" \\[(\\d+)\\] (\\d+) \"([^\"]*)\" \"([^\"]*)\"");

    /**
     * 解析单行日志记录
     * @param logLine 日志行
     * @return 包含解析字段的Map
     */
    public static Map<String, String> parseLogLine(String logLine) {
        Map<String, String> logMap = new HashMap<>();
        
        Matcher matcher = LOG_PATTERN.matcher(logLine);
        if (matcher.find()) {
            logMap.put("remote_addr", matcher.group(1));
            logMap.put("remote_user", matcher.group(2) + " " + matcher.group(3));
            
            // 组合日期和时间
            String date = matcher.group(4);
            String hour = matcher.group(5);
            String second = matcher.group(6);
            logMap.put("time_local", date + ":" + hour + ":" + second);
            logMap.put("hour", hour);
            logMap.put("date_hour", convertDateFormat(date) + hour);
            
            // 请求信息
            logMap.put("method", matcher.group(7));
            logMap.put("request", matcher.group(8));
            logMap.put("protocol", matcher.group(9));
            
            // 状态码和传输字节
            logMap.put("status", matcher.group(10));  // 注意：这里取出的带方括号的状态码
            logMap.put("body_bytes_sent", matcher.group(11));
            
            // 引用页和用户代理
            logMap.put("http_referer", matcher.group(12));
            logMap.put("http_user_agent", matcher.group(13));
            
            // 提取浏览器类型
            logMap.put("browser_type", extractBrowserType(matcher.group(13)));
            return logMap;
        } else {
            // 如果无法匹配，尝试更宽松的匹配
            Pattern fallbackPattern = Pattern.compile("(\\S+) (\\S+) (\\S+) \\[([^\\]]+)\\] \"([^\"]+)\" \\[(\\d+)\\] (\\d+) \"([^\"]*)\" \"([^\"]*)\"");
            matcher = fallbackPattern.matcher(logLine);
            
            if (matcher.find()) {
                logMap.put("remote_addr", matcher.group(1));
                logMap.put("remote_user", matcher.group(2) + " " + matcher.group(3));
                
                // 提取时间
                String timeStr = matcher.group(4);
                logMap.put("time_local", timeStr);
                
                // 尝试提取小时信息，如果格式允许
                String[] timeParts = timeStr.split(":");
                if (timeParts.length >= 2) {
                    logMap.put("hour", timeParts[1]);
                    String[] dateParts = timeParts[0].split("/");
                    if (dateParts.length >= 3) {
                        String date = dateParts[0] + "/" + dateParts[1] + "/" + dateParts[2];
                        logMap.put("date_hour", convertDateFormat(date) + timeParts[1]);
                    }
                }
                
                // 请求信息 - 整个请求字符串
                String requestStr = matcher.group(5);
                logMap.put("request", requestStr);
                
                // 尝试分解请求字符串
                String[] requestParts = requestStr.split(" ");
                if (requestParts.length >= 3) {
                    logMap.put("method", requestParts[0]);
                    logMap.put("url", requestParts[1]);
                    logMap.put("protocol", requestParts[2]);
                }
                
                // 状态码和传输字节
                logMap.put("status", matcher.group(6));
                logMap.put("body_bytes_sent", matcher.group(7));
                
                // 引用页和用户代理
                logMap.put("http_referer", matcher.group(8));
                logMap.put("http_user_agent", matcher.group(9));
                
                // 提取浏览器类型
                logMap.put("browser_type", extractBrowserType(matcher.group(9)));
                return logMap;
            } else {
                // 如果所有尝试都失败，则记录错误
                logMap.put("error", "Failed to parse log line: " + logLine);
                return logMap;
            }
        }
    }
    
    /**
     * 从User-Agent字符串中提取浏览器类型
     * @param userAgent User-Agent字符串
     * @return 浏览器类型
     */
    private static String extractBrowserType(String userAgent) {
        if (userAgent == null || userAgent.isEmpty()) {
            return "Unknown";
        }
        
        // 简单的浏览器类型提取逻辑
        if (userAgent.contains("Chrome")) {
            return "Chrome";
        } else if (userAgent.contains("Firefox")) {
            return "Firefox";
        } else if (userAgent.contains("Safari") && !userAgent.contains("Chrome")) {
            return "Safari";
        } else if (userAgent.contains("MSIE") || userAgent.contains("Trident")) {
            return "Internet Explorer";
        } else if (userAgent.contains("Opera") || userAgent.contains("OPR")) {
            return "Opera";
        } else if (userAgent.contains("Mobile") || userAgent.contains("Android")) {
            return "Mobile Browser";
        } else if (userAgent.contains("bot") || userAgent.contains("Bot") || 
                userAgent.contains("spider") || userAgent.contains("Spider") ||
                userAgent.contains("Monitor")) {
            return "Web Bot";
        } else {
            return "Other";
        }
    }
    
    /**
     * 将日期格式从 18/Sep/2013 转换为 20130918
     * @param dateStr 原始日期字符串
     * @return 转换后的日期格式
     */
    private static String convertDateFormat(String dateStr) {
        Map<String, String> monthMap = new HashMap<>();
        monthMap.put("Jan", "01");
        monthMap.put("Feb", "02");
        monthMap.put("Mar", "03");
        monthMap.put("Apr", "04");
        monthMap.put("May", "05");
        monthMap.put("Jun", "06");
        monthMap.put("Jul", "07");
        monthMap.put("Aug", "08");
        monthMap.put("Sep", "09");
        monthMap.put("Oct", "10");
        monthMap.put("Nov", "11");
        monthMap.put("Dec", "12");
        
        String[] parts = dateStr.split("/");
        if (parts.length == 3) {
            String day = parts[0];
            String month = monthMap.getOrDefault(parts[1], "01");
            String year = parts[2];
            
            // 确保天是两位数
            if (day.length() == 1) {
                day = "0" + day;
            }
            
            return year + month + day;
        }
        
        return "Unknown";
    }
} 