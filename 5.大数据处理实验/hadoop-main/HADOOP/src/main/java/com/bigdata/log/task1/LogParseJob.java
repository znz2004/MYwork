package com.bigdata.log.task1;

import java.io.IOException;
import java.util.Map;

import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;

import com.bigdata.log.utils.LogParser;

/**
 * 任务1: 日志解析与清洗
 */
public class LogParseJob {

    /**
     * Mapper类: 解析输入的日志行
     */
    public static class LogParseMapper extends Mapper<LongWritable, Text, Text, Text> {
        private Text outKey = new Text();
        private Text outValue = new Text();
        
        @Override
        protected void map(LongWritable key, Text value, Context context)
                throws IOException, InterruptedException {
            // 获取日志行内容
            String line = value.toString();
            
            // 过滤空行或格式不正确的行
            if (line == null || line.trim().isEmpty()) {
                return;
            }
            
            try {
                // 使用LogParser工具类解析日志行
                Map<String, String> logMap = LogParser.parseLogLine(line);
                
                // 检查解析是否成功
                if (logMap.containsKey("error")) {
                    // 解析失败，记录错误
                    context.getCounter("Log Parser", "Failed to parse").increment(1);
                    return;
                }
                
                // 解析成功，生成输出
                // 使用日志行的序号作为键
                outKey.set(key.toString());
                
                // 构建包含解析结果的值
                StringBuilder sb = new StringBuilder();
                sb.append("remote_addr:").append(logMap.get("remote_addr")).append("\n");
                sb.append("remote_user:").append(logMap.get("remote_user")).append("\n");
                sb.append("time_local:").append(logMap.get("time_local")).append("\n");
                sb.append("request:").append(logMap.get("request")).append("\n");
                sb.append("status:").append(logMap.get("status")).append("\n");
                sb.append("body_bytes_sent:").append(logMap.get("body_bytes_sent")).append("\n");
                sb.append("http_referer:").append(logMap.get("http_referer")).append("\n");
                sb.append("http_user_agent:").append(logMap.get("http_user_agent")).append("\n");
                sb.append("browser_type:").append(logMap.get("browser_type"));
                
                outValue.set(sb.toString());
                context.write(outKey, outValue);
                
                // 记录成功解析的日志数量
                context.getCounter("Log Parser", "Successfully parsed").increment(1);
            } catch (Exception e) {
                // 记录解析异常
                context.getCounter("Log Parser", "Parse exception").increment(1);
            }
        }
    }

    /**
     * Reducer类: 收集解析结果
     */
    public static class LogParseReducer extends Reducer<Text, Text, Text, Text> {
        @Override
        protected void reduce(Text key, Iterable<Text> values, Context context)
                throws IOException, InterruptedException {
            // 由于每个日志行的键是唯一的（行号），因此values迭代器中只会有一个元素
            for (Text value : values) {
                context.write(key, value);
            }
        }
    }
} 