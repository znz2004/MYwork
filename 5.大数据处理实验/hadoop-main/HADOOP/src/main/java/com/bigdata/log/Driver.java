package com.bigdata.log;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;

import com.bigdata.log.task1.LogParseJob;
import com.bigdata.log.task2.HourlyVisitJob;
import com.bigdata.log.task2.BrowserTypeJob;
import com.bigdata.log.task3.StatusBytesJob;
import com.bigdata.log.task3.HourlyErrorRateJob;
import com.bigdata.log.task3.BrowserErrorRateJob;
import com.bigdata.log.task4.UserFeatureJob;
import com.bigdata.log.task4.UserClusteringJob;

/**
 * 应用程序主类，负责处理命令行参数并执行相应的MapReduce任务
 */
public class Driver {
    public static void main(String[] args) throws Exception {
        if (args.length < 3) {
            System.err.println("Usage: LogAnalyzer <task> <input_path> <output_path>");
            System.err.println("Tasks:");
            System.err.println("  parse: 日志解析与清洗 (任务1)");
            System.err.println("  hourly: 每小时网站浏览量统计 (任务2-1)");
            System.err.println("  browser: 用户端类型统计 (任务2-2)");
            System.err.println("  status_bytes: 状态码与传输数据量关系分析 (任务3-1)");
            System.err.println("  error_rate: 小时级错误率波动分析 (任务3-2)");
            System.err.println("  browser_error: 高风险用户类型识别 (任务3-3)");
            System.err.println("  feature: 用户行为特征提取 (任务4-1)");
            System.err.println("  cluster: 用户分群 (任务4-2)");
            System.exit(1);
        }

        String task = args[0];
        String inputPath = args[1];
        String outputPath = args[2];

        boolean success = false;

        // 根据任务参数执行相应的MapReduce作业
        switch (task) {
            case "parse":
                success = runLogParseJob(inputPath, outputPath);
                break;
            case "hourly":
                success = runHourlyVisitJob(inputPath, outputPath);
                break;
            case "browser":
                success = runBrowserTypeJob(inputPath, outputPath);
                break;
            case "status_bytes":
                success = runStatusBytesJob(inputPath, outputPath);
                break;
            case "error_rate":
                success = runErrorRateJob(inputPath, outputPath);
                break;
            case "browser_error":
                success = runBrowserErrorRateJob(inputPath, outputPath);
                break;
            case "feature":
                success = runUserFeatureJob(inputPath, outputPath);
                break;
            case "cluster":
                success = runUserClusteringJob(inputPath, outputPath);
                break;
            default:
                System.err.println("Unknown task: " + task);
                System.exit(1);
        }

        System.exit(success ? 0 : 1);
    }

    /**
     * 运行任务1: 日志解析与清洗
     */
    private static boolean runLogParseJob(String inputPath, String outputPath) throws Exception {
        Configuration conf = new Configuration();
        Job job = Job.getInstance(conf, "Log Parse Job");
        job.setJarByClass(Driver.class);
        
        // 设置Mapper和Reducer类
        job.setMapperClass(LogParseJob.LogParseMapper.class);
        job.setReducerClass(LogParseJob.LogParseReducer.class);
        
        // 设置输出键值对类型
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        
        // 设置输入和输出路径
        FileInputFormat.addInputPath(job, new Path(inputPath));
        FileOutputFormat.setOutputPath(job, new Path(outputPath));
        
        // 设置Reducer数量
        job.setNumReduceTasks(2);
        
        return job.waitForCompletion(true);
    }

    /**
     * 运行任务2-1: 每小时网站浏览量统计
     */
    private static boolean runHourlyVisitJob(String inputPath, String outputPath) throws Exception {
        Configuration conf = new Configuration();
        Job job = Job.getInstance(conf, "Hourly Visit Job");
        job.setJarByClass(Driver.class);
        
        // 设置Mapper和Reducer类
        job.setMapperClass(HourlyVisitJob.HourlyVisitMapper.class);
        job.setReducerClass(HourlyVisitJob.HourlyVisitReducer.class);
        
        // 设置输出键值对类型
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        
        // 设置输入和输出路径
        FileInputFormat.addInputPath(job, new Path(inputPath));
        FileOutputFormat.setOutputPath(job, new Path(outputPath));
        
        // 设置Reducer数量
        job.setNumReduceTasks(2);
        
        return job.waitForCompletion(true);
    }

    /**
     * 运行任务2-2: 用户端类型统计
     */
    private static boolean runBrowserTypeJob(String inputPath, String outputPath) throws Exception {
        Configuration conf = new Configuration();
        Job job = Job.getInstance(conf, "Browser Type Job");
        job.setJarByClass(Driver.class);
        
        // 设置Mapper和Reducer类
        job.setMapperClass(BrowserTypeJob.BrowserTypeMapper.class);
        job.setReducerClass(BrowserTypeJob.BrowserTypeReducer.class);
        
        // 设置输出键值对类型
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        
        // 设置输入和输出路径
        FileInputFormat.addInputPath(job, new Path(inputPath));
        FileOutputFormat.setOutputPath(job, new Path(outputPath));
        
        // 设置Reducer数量
        job.setNumReduceTasks(2);
        
        return job.waitForCompletion(true);
    }

    /**
     * 运行任务3-1: 状态码与传输数据量关系分析
     */
    private static boolean runStatusBytesJob(String inputPath, String outputPath) throws Exception {
        Configuration conf = new Configuration();
        Job job = Job.getInstance(conf, "Status Bytes Job");
        job.setJarByClass(Driver.class);
        
        // 设置Mapper和Reducer类
        job.setMapperClass(StatusBytesJob.StatusBytesMapper.class);
        job.setReducerClass(StatusBytesJob.StatusBytesReducer.class);
        
        // 设置输出键值对类型
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        
        // 设置输入和输出路径
        FileInputFormat.addInputPath(job, new Path(inputPath));
        FileOutputFormat.setOutputPath(job, new Path(outputPath));
        
        // 设置Reducer数量
        job.setNumReduceTasks(3);
        
        return job.waitForCompletion(true);
    }

    /**
     * 运行任务3-2: 小时级错误率波动分析
     */
    private static boolean runErrorRateJob(String inputPath, String outputPath) throws Exception {
        Configuration conf = new Configuration();
        Job job = Job.getInstance(conf, "Error Rate Job");
        job.setJarByClass(Driver.class);
        
        // 设置Mapper和Reducer类
        job.setMapperClass(HourlyErrorRateJob.ErrorRateMapper.class);
        job.setReducerClass(HourlyErrorRateJob.ErrorRateReducer.class);
        
        // 设置输出键值对类型
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        
        // 设置输入和输出路径
        FileInputFormat.addInputPath(job, new Path(inputPath));
        FileOutputFormat.setOutputPath(job, new Path(outputPath));
        
        // 设置Reducer数量
        job.setNumReduceTasks(3);
        
        return job.waitForCompletion(true);
    }

    /**
     * 运行任务3-3: 高风险用户类型识别
     */
    private static boolean runBrowserErrorRateJob(String inputPath, String outputPath) throws Exception {
        Configuration conf = new Configuration();
        Job job = Job.getInstance(conf, "Browser Error Rate Job");
        job.setJarByClass(Driver.class);
        
        // 设置Mapper和Reducer类
        job.setMapperClass(BrowserErrorRateJob.BrowserErrorRateMapper.class);
        job.setReducerClass(BrowserErrorRateJob.BrowserErrorRateReducer.class);
        
        // 设置输出键值对类型
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        
        // 设置输入和输出路径
        FileInputFormat.addInputPath(job, new Path(inputPath));
        FileOutputFormat.setOutputPath(job, new Path(outputPath));
        
        // 设置Reducer数量
        job.setNumReduceTasks(3);
        
        return job.waitForCompletion(true);
    }

    /**
     * 运行任务4-1: 用户行为特征提取
     */
    private static boolean runUserFeatureJob(String inputPath, String outputPath) throws Exception {
        Configuration conf = new Configuration();
        Job job = Job.getInstance(conf, "User Feature Job");
        job.setJarByClass(Driver.class);
        
        // 设置Mapper和Reducer类
        job.setMapperClass(UserFeatureJob.UserFeatureMapper.class);
        job.setReducerClass(UserFeatureJob.UserFeatureReducer.class);
        
        // 设置输出键值对类型
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        
        // 设置输入和输出路径
        FileInputFormat.addInputPath(job, new Path(inputPath));
        FileOutputFormat.setOutputPath(job, new Path(outputPath));
        
        // 设置Reducer数量
        job.setNumReduceTasks(4);
        
        return job.waitForCompletion(true);
    }

    /**
     * 运行任务4-2: 用户分群
     */
    private static boolean runUserClusteringJob(String inputPath, String outputPath) throws Exception {
        Configuration conf = new Configuration();
        Job job = Job.getInstance(conf, "User Clustering Job");
        job.setJarByClass(Driver.class);
        
        // 设置Mapper和Reducer类
        job.setMapperClass(UserClusteringJob.UserClusteringMapper.class);
        job.setReducerClass(UserClusteringJob.UserClusteringReducer.class);
        
        // 设置输出键值对类型
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        
        // 设置输入和输出路径
        FileInputFormat.addInputPath(job, new Path(inputPath));
        FileOutputFormat.setOutputPath(job, new Path(outputPath));
        
        // 设置Reducer数量
        job.setNumReduceTasks(4);
        
        return job.waitForCompletion(true);
    }
} 