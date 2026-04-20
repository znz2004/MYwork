#include "nit.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include "apis.h"

Blob::Blob()
{
	m_content.clear();
	m_content_hash.clear();
}

Blob::Blob(const std::string &content)
{
	m_content = content;
	m_content_hash = UsefulApi::hash(content);
}


std::string Blob::GetHash() const
{
	return m_content_hash;
}


std::string Blob::GetContent() const
{
	return m_content;
}


Tree::Tree()
{
	m_files.clear();
}


bool Tree::AddFile(const std::string &filename, const Blob &blob)
{
	std::map<std::string, Blob>::iterator iter = m_files.find(filename);
	if(iter != m_files.end())
	{
		return false;
	}

	m_files[filename] = blob;
	return true;
}


void Tree::RemoveFile(const std::string& filename)
{
	std::map<std::string, Blob>::iterator iter = m_files.find(filename);
	if (iter == m_files.end())
	{
		return;
	}

	m_files.erase(iter);
	return;
}


std::map<std::string, Blob>& Tree::GetFiles()
{
	return m_files;
}


CCommit::CCommit()
{
	m_message.clear();
	m_parent = NULL;
	m_commit_hash.clear();
}


CCommit::CCommit(const std::string &msg, const Tree &tree, CCommit *parent /*= NULL*/)
	:m_tree(tree)
	,m_message(msg)
	,m_parent(parent)
{
	m_commit_hash = hash(msg + (parent ? parent->m_commit_hash : ""));
}


std::string CCommit::GetHash() const
{
	return m_commit_hash;
}


std::string CCommit::GetMessageSelf() const
{
	return m_message;
}


CCommit* CCommit::GetParent() const
{
	return m_parent;
}


Tree& CCommit::GetTree()
{
	return m_tree;
}


Nit::Nit()
{
	m_cwd_path = UsefulApi::cwd();
	m_nit_dir = m_cwd_path + "/.nit";
	m_head_path = m_nit_dir + "/HEAD";
	m_index_path = m_nit_dir + "/index";
	m_remove_path = m_nit_dir + "/remove";
	m_commit_path = m_nit_dir + "/commits";
	m_head_commit = NULL;
}


bool Nit::IsNitRepo()
{
	return std::filesystem::exists(m_nit_dir);
}


void Nit::Init()
{
	if (IsNitRepo())
	{
		std::cout << "A Nit version-control system already exists in the current directory." << std::endl;
		return;
	}

	std::filesystem::create_directory(m_nit_dir);
	std::filesystem::create_directory(m_commit_path);

	Tree emptyTree;
	m_head_commit = new CCommit("initial commit", emptyTree);
	SaveCommit(m_head_commit);

	UsefulApi::writeToFile(m_head_commit->GetHash(), m_head_path);
}


void Nit::Add(const std::string &filename)
{
	std::map<std::string, Blob> staging_area_files = m_staging_area.GetFiles();
	std::string file_path = m_cwd_path + "/" + filename;
	Blob blob;
	if (!std::filesystem::exists(file_path))
	{		
		std::map<std::string, Blob>::iterator iter_file = staging_area_files.find(filename);
		if (iter_file == staging_area_files.end())
		{
			std::cout << "File does not exist." << std::endl;
			return;
		}

		m_removed_files.AddFile(filename, iter_file->second);
		m_staging_area.RemoveFile(filename);
		SaveStagineArea();
		SaveRemoveFileArea();
	}
	else
	{
		std::string content;
		if (!UsefulApi::readFromFile(file_path, content))
		{
			std::cout << "File does not exist." << std::endl;
			return;
		}

		Blob blob(content);
		if (m_staging_area.AddFile(filename, blob))
		{
			SaveStagineArea();
		}
	}
}


void Nit::Commit(const std::string &msg)
{
	if (m_staging_area.GetFiles().empty() && m_removed_files.GetFiles().empty())
	{
		std::cout << "No changes added to the commit." << std::endl;
		return;
	}

	m_head_commit = new CCommit(msg, m_staging_area, m_head_commit);
	SaveCommit(m_head_commit);
	UsefulApi::writeToFile(m_head_commit->GetHash(), m_head_path);
	ClearStagineArea();
}


void Nit::Status()
{
	std::cout << "=== Staged Files ===" << std::endl;
	std::map<std::string, Blob> files = m_staging_area.GetFiles();
	for (std::map<std::string, Blob>::iterator iter = files.begin(); iter != files.end(); ++iter)
	{
		std::cout << iter->first << std::endl;
	}

	std::cout << "=== Removed Files ===" << std::endl;
	files = m_removed_files.GetFiles();
	for (std::map<std::string, Blob>::iterator iter = files.begin(); iter != files.end(); ++iter)
	{
		std::cout << iter->first << std::endl;
	}
}


void Nit::Checkout(const std::string &commitId)
{
	CCommit *target_commit = LoadCommit(commitId);
	if (!target_commit)
	{
		std::cout << "No commit with that id exists." << std::endl;
		return;
	}

	Tree &target_tree = target_commit->GetTree();
	std::map<std::string, Blob> target_files = target_tree.GetFiles();

	std::string cur_commit_hash;
	if (UsefulApi::readFromFile(m_head_path, cur_commit_hash))
	{
		CCommit* cur_commit = LoadCommit(cur_commit_hash);
		if (cur_commit != NULL)
		{
			for (std::map<std::string, Blob>::iterator iter = target_files.begin(); iter != target_files.end(); ++iter)
			{
				Tree& tmp_tree = cur_commit->GetTree();
				std::map<std::string, Blob> tmp_files = tmp_tree.GetFiles();
				std::map<std::string, Blob>::iterator iter_tmp = tmp_files.find(iter->first);
				if (iter_tmp == tmp_files.end())
				{
					std::cout << "There is an untracked file in the way; delete it, or add and commit it first." << std::endl;
					return;
				}
			}
		}
	}
	
	LoadAllCommit();
	CCommit *cur_commit = m_head_commit;
	while (cur_commit)
	{
		Tree &tmp_tree = cur_commit->GetTree();
		std::map<std::string, Blob> tmp_files = tmp_tree.GetFiles();
		for (std::map<std::string, Blob>::iterator iter = tmp_files.begin(); iter != tmp_files.end(); ++iter)
		{
			std::map<std::string, Blob>::iterator iter_target = target_files.find(iter->first);
			if (iter_target == target_files.end())
			{
				std::filesystem::remove(m_cwd_path + "/" + iter->first);
			}
		}
		cur_commit = cur_commit->GetParent();
	}

	for (std::map<std::string, Blob>::iterator iter = target_files.begin(); iter != target_files.end(); ++iter)
	{
		UsefulApi::writeToFile(iter->second.GetContent(), m_cwd_path + "/" + iter->first);
	}

	m_head_commit = target_commit;
	ClearStagineArea();
	UsefulApi::writeToFile(m_head_commit->GetHash(), m_head_path);
}


void Nit::Log()
{
	CCommit *cur_commit = m_head_commit;
	while (cur_commit)
	{
		std::cout << "=== commit " << cur_commit->GetHash() << std::endl;
		std::cout << cur_commit->GetMessageSelf() << std::endl;
		cur_commit = cur_commit->GetParent();
	}
}


void Nit::LoadAllCommit()
{
	std::vector<std::string> files = UsefulApi::listFilesInDirectory(m_commit_path);

	CCommit *parent = NULL;
	for (int i = 0; i < (int)files.size(); ++i)
	{
		if (files[i].find("_hash") == std::string::npos)
		{
			continue;
		}

		size_t end = files[i].find('_');
		std::string hash = files[i].substr(0, end);

		CCommit *new_commit = LoadCommit(hash);
		if (new_commit != NULL)
		{
			new_commit->SetParent(parent);
			parent = new_commit;
		}
	}

	if (m_head_commit == NULL)
	{
		m_head_commit = parent;
	}
}


CCommit *Nit::LoadCommit(const std::string &commitId)
{
	std::string commit_path = m_commit_path + "/" + commitId + "_hash";
	if (!std::filesystem::exists(commit_path))
	{
		return NULL;
	}

	std::string message;
	UsefulApi::readFromFile(commit_path, message);

	Tree file_tree;
	std::vector<std::string> files = UsefulApi::listFilesInDirectory(m_commit_path);
	for (int i = files.size() - 1; i >= 0; --i)
	{
		if (files[i].find(commitId) != std::string::npos  && files[i].find("_hash") == std::string::npos)
		{
			std::string file_content;
			if (UsefulApi::readFromFile(m_commit_path + "/" + files[i], file_content))
			{
				size_t start = files[i].find('_');

				Blob blob(file_content);
				file_tree.AddFile(files[i].substr(start + 1), blob);
			}
		}
	}

	return new CCommit(message, file_tree, NULL);
}


void Nit::SaveCommit(CCommit* commit)
{
	std::string commit_path = m_commit_path + "/" + commit->GetHash() + "_hash";
	UsefulApi::writeToFile(commit->GetMessageSelf(), commit_path);

	std::map<std::string, Blob> files = commit->GetTree().GetFiles();
	for (std::map<std::string, Blob>::iterator iter = files.begin(); iter != files.end(); ++iter)
	{
		std::string commit_file_path = m_commit_path + "/" + commit->GetHash() + "_" + iter->first;
		UsefulApi::writeToFile(iter->second.GetContent(), commit_file_path);
	}
}


void Nit::SaveStagineArea()
{
	std::string str;
	std::map<std::string, Blob> files = m_staging_area.GetFiles();
	for (std::map<std::string, Blob>::iterator iter = files.begin(); iter != files.end(); ++iter)
	{
		str += iter->first + " " + iter->second.GetHash() + "\n";
	}
	UsefulApi::writeToFile(str, m_index_path);
}


void Nit::LoadStagineArea()
{
	if (!std::filesystem::exists(m_index_path))
	{
		return;
	}

	std::ifstream index_file(m_index_path);
	std::string filename, hash;
	while (index_file >> filename >> hash)
	{
		std::string file_content;
		UsefulApi::readFromFile(filename, file_content);
		m_staging_area.AddFile(filename, Blob(file_content));
	}
}


void Nit::ClearStagineArea()
{
	m_staging_area.GetFiles().clear();
	m_removed_files.GetFiles().clear();
	std::ofstream index_file(m_index_path, std::ios::trunc);
	index_file.close();
}


void Nit::LoadRemoveFileArea()
{
	if (!std::filesystem::exists(m_remove_path))
	{
		return;
	}

	std::ifstream index_file(m_remove_path);
	std::string filename, hash;
	while (index_file >> filename >> hash)
	{
		m_removed_files.AddFile(filename, Blob());
	}
}


void Nit::SaveRemoveFileArea()
{
	std::string str;
	std::map<std::string, Blob> files = m_removed_files.GetFiles();
	for (std::map<std::string, Blob>::iterator iter = files.begin(); iter != files.end(); ++iter)
	{
		str += iter->first + " " + iter->second.GetHash() + "\n";
	}
	UsefulApi::writeToFile(str, m_remove_path);
}





