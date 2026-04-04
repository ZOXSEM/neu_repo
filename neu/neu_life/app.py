import os, time, sys, json, re
from github import Github, Auth
from huggingface_hub import InferenceClient

def log(msg, status="ИНФО"):
    print(f"[{status}] {time.strftime('%H:%M:%S')} >> {msg}", flush=True)

log("СИСТЕМА_ЗАПУЩЕНА")

try:
    auth = Auth.Token(os.environ.get("GITHUB_TOKEN"))
    g = Github(auth=auth)
    repo = g.get_repo(os.environ.get("REPO_PATH"))
    client = InferenceClient(model="Qwen/Qwen2.5-Coder-32B-Instruct", token=os.environ.get("HF_TOKEN"))

    def get_valid_content(filename, default=""):
        try:
            file = repo.get_contents(filename)
            content = file.decoded_content.decode("utf-8").strip()
            return file, content
        except:
            log(f"РЕГЕНЕРАЦИЯ_ФАЙЛА: {filename}", "ВНИМАНИЕ")
            repo.create_file(filename, f"init: {filename}", default)
            return repo.get_contents(filename), default

    def extract_text(resp):
        try:
            if hasattr(resp, 'choices'): return resp.choices[0].message.content
            if isinstance(resp, list): return str(resp[0])
            return str(resp)
        except: return str(resp)

    def process_cycle():
        gen_obj, general_goal = get_valid_content("general.txt", "Create 2D top-down racing engine.")
        add_obj, addition = get_valid_content("addition.txt", "")
        inst_json_obj, inst_raw = get_valid_content("instruction.json", '{"history": [], "next_goal": "Setup base engine"}')
        
        try: 
            plan = json.loads(inst_raw)
            if "history" not in plan: plan["history"] = []
        except: 
            plan = {"history": [], "next_goal": "Setup base engine"}

        main_obj, current_code = get_valid_content("main.cpp", "// INITIAL_VOID")

        if "TEST_NEU_END" in addition:
            log("ФИНАЛЬНАЯ_ПОЛИРОВКА", "FINAL")
            p = f"Polish the project. No new features. Fix bugs. Source: {current_code}"
            resp = client.chat_completion(messages=[{"role": "user", "content": p}], max_tokens=4000)
            final_code = extract_text(resp).replace("```cpp", "").replace("```", "").strip()
            repo.update_file("main.cpp", "final: stable_release", final_code, main_obj.sha)
            log("ПРОЕКТ_ЗАВЕРШЕН. ВЫКЛЮЧЕНИЕ.", "STOP")
            sys.exit(0)

        log("ГЕНЕРАЦИЯ_КОДА_И_ПЛАНА")
        
        prompt = f"""
        ACT_AS: SENIOR_GAME_DEVELOPER
        STRATEGY: {general_goal}
        ADDITIONS: {addition if addition else "None"}
        SOURCE: {current_code}
        INTERNAL_PLAN: {plan.get('next_goal')}

        STRICT_FORMAT:
        - Code in [CODE]...[/CODE]
        - History in [HISTORY]...[/HISTORY]
        - Next technical goal in [NEXT_GOAL]...[/NEXT_GOAL]
        """
        
        raw_resp = client.chat_completion(messages=[{"role": "user", "content": prompt}], max_tokens=4000)
        full_text = extract_text(raw_resp)

        try:
            new_code = re.search(r'\[CODE\](.*?)\[/CODE\]', full_text, re.S).group(1)
        except:
            new_code = re.search(r'```cpp(.*?)```', full_text, re.S).group(1) if "```" in full_text else full_text

        try:
            history_note = re.search(r'\[HISTORY\](.*?)\[/HISTORY\]', full_text, re.S).group(1)
            next_goal = re.search(r'\[NEXT_GOAL\](.*?)\[/NEXT_GOAL\]', full_text, re.S).group(1)
        except:
            history_note = "Автономная эволюция кода."
            next_goal = "Continue development"

        new_code = new_code.replace("```cpp", "").replace("```", "").strip()

        repo.update_file("main.cpp", "feat: autonomous_evolution", new_code, main_obj.sha)
        
        plan["history"].append(history_note)
        plan["next_goal"] = next_goal
        repo.update_file(inst_json_obj.path, "sys: plan_sync", json.dumps(plan, indent=4), inst_json_obj.sha)
        
        hist_obj, hist_content = get_valid_content("history.md", "# 📜 Лог разработки\n")
        repo.update_file(hist_obj.path, "docs: history_upd", f"{hist_content}\n\n### {time.strftime('%H:%M')} >> {history_note}", hist_obj.sha)

        if addition:
            lines = addition.split('\n')
            new_addition = "\n".join(lines[1:]) if len(lines) > 1 else ""
            repo.update_file(add_obj.path, "sys: task_shift", new_addition, add_obj.sha)

    while True:
        try:
            process_cycle()
            log("ЦИКЛ_ОКОНЧЕН. СОН_600С.")
            time.sleep(600)
        except Exception as e:
            log(f"ОШИБКА: {str(e)}", "СБОЙ")
            time.sleep(60)

except Exception as e:
    log(f"CRITICAL: {str(e)}", "ОСТАНОВКА")
