#ifndef NIT_H
#define NIT_H

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <filesystem>
#include "apis.h"

using namespace UsefulApi;

class Blob
{
public:
	Blob();
	Blob(const std::string &content);
	std::string GetHash() const;
	std::string GetContent() const;

private:
	std::string m_content;
	std::string m_content_hash;
};



class Tree
{
public:
	Tree();

	bool AddFile(const std::string &filename, const Blob &blob);
	void RemoveFile(const std::string& filename);

	std::map<std::string, Blob> &GetFiles();


private:
	std::map<std::string, Blob> m_files;
};


class CCommit
{
public:
	CCommit();
	CCommit(const std::string &msg, const Tree &tree, CCommit *parent = NULL);

	std::string GetHash() const;
	std::string GetMessageSelf() const;
	CCommit* GetParent() const;
	void SetParent(CCommit* parent) { m_parent = parent; }
	Tree &GetTree();

private:
	std::string m_commit_hash;
	Tree m_tree;
	std::string m_message;
	CCommit *m_parent;
};

class Commit;
class Nit
{
public:
	Nit();
	void Init();
	void Add(const std::string &filename);
	void Commit(const std::string &msg);
	void Status();
	void Checkout(const std::string &commitId);
	void Log();
	bool IsNitRepo();
	void LoadAllCommit();
	CCommit *LoadCommit(const std::string &commitId);
	void SaveCommit(CCommit *commit);
	void LoadStagineArea();
	void SaveStagineArea();
	void ClearStagineArea();
	void LoadRemoveFileArea();
	void SaveRemoveFileArea();

private:
	std::string m_cwd_path;
	std::string m_nit_dir;
	std::string m_head_path;
	std::string m_index_path;
	std::string m_remove_path;
	std::string m_commit_path;
	CCommit *m_head_commit;
	Tree m_staging_area;
	Tree m_removed_files;
};



#endif
