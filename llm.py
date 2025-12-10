import subprocess, time, json, os, argparse
from openai import OpenAI
from openai.types.responses import Response

def run_python(code: str, timeout: int = 300):
    start = time.time()
    try:
        proc = subprocess.run(
            ['python', '-c', code],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            encoding='utf-8',
            timeout=timeout
        )
        duration = time.time() - start
        return {
            "stdout": proc.stdout,
            "stderr": proc.stderr,
            "return code": proc.returncode,
            "duration": duration
        }
    except subprocess.TimeoutExpired as e:
        return {"stdout": "", "stderr": f"Timeout after {timeout}s", "returncode": -1, "duration": time.time() - start}
    except Exception as e:
        return {"stdout": "", "stderr": f"Execution error: {e}", "returncode": -2, "duration": time.time() - start}

def run_mathematica(code: str, timeout: int = 300):
    start = time.time()
    try:
        proc = subprocess.run(
            ['wolframscript', '-code', code],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            encoding='utf-8',
            timeout=timeout
        )
        duration = time.time() - start
        return {
            "stdout": proc.stdout,
            "stderr": proc.stderr,
            "return code": proc.returncode,
            "duration": duration
        }
    except subprocess.TimeoutExpired as e:
        return {"stdout": "", "stderr": f"Timeout after {timeout}s", "returncode": -1, "duration": time.time() - start}
    except Exception as e:
        return {"stdout": "", "stderr": f"Execution error: {e}", "returncode": -2, "duration": time.time() - start}

API_URL = "API_URL"
API_KEY = "API_KEY"
MODEL_NAME = "gpt-5"

tools = [
    {
        "type": "function",
        "name": "run_mathematica",
        "description": """\
You can use this function to run a Wolfram Language (Mathematica) code and get the result (stdout/stderr).
For example, given `a`, if you want to calculate the maximum `y` such that there exists a `x` satisfying `x>0 && x^2+a*x*y+y^2=1`, you can run the code `Maximize[{y, x^2+a*x*y+y^2==1 && x>0}, {x, y}]`.
If you don't know how to use a function FUNC in Wolfram Language, you can run `?FUNC` to see the usage.
The result will be given to you in JSON format. There are four properties: stdout, stderr, return code, duration. In the above example, the result is {"stdout": "{Piecewise[{{1, a >= 0}, {2*Sqrt[-(-4 + a^2)^(-1)], Inequality[-2, Less, a, Less, 0]}}, Infinity], {x -> Piecewise[{{-(a*Sqrt[-(-4 + a^2)^(-1)]), Inequality[-2, Less, a, Less, 0]}}, Indeterminate], y -> Piecewise[{{1, a >= 0}, {2*Sqrt[-(-4 + a^2)^(-1)], Inequality[-2, Less, a, Less, 0]}}, Infinity]}}\n", "stderr": "", "return code": 0, "duration": 1.5105085372924805}.
If you can use python to do the calculation yourself, you don't need to call this function. You may call this function multiple times to solve a complicated problem. In every single calling, only the result of the last line of the command will be returned.
Note that all calls are independent; each call does not inherit anything from previous calls. Therefore, variables that need to be reused must be redefined in each call.
""",
        "parameters": {
            "type": "object",
            "properties": {
                "code": {
                    "type": "string",
                    "description": "The Wolfram Language code you want to run. For example, `Maximize[{y, x^2+a*x*y+y^2==1 && x>0}, {x, y}]`."
                }
            }
        },
        "required": ["code"]
    },
    {
        "type": "function",
        "name": "run_python",
        "description": "You can use this function to run any python code and get the result (stdout/stderr).",
        "parameters": {
            "type": "object",
            "properties": {
                "code": {
                    "type": "string",
                    "description": "The python code you want to run."
                }
            }
        }
    }
]

function_map = {
    "run_mathematica": run_mathematica,
    "run_python": run_python,
}

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('--splus', action='store_true', help='find a new s_plus, read prompt from splus_prompt.txt')
    parser.add_argument('--regular-point', action='store_true', help='find a new regular point, read prompt from regular_point_prompt.txt')

    args = parser.parse_args()
    if not args.splus and not args.regular_point:
        print('At least one of --splus and --regular-point should be set.')
        print('Run `python llm.py --help` to see the usage.')
        exit(0)
    elif args.splus and args.regular_point:
        print('Only one of --splus and --regular-point should be set.')
        print('Run `python llm.py --help` to see the usage.')
        exit(0)

    if args.splus:
        prompt_file = 'splus_prompt.txt'
    elif args.regular_point:
        prompt_file = 'regular_point_prompt.txt'

    with open(prompt_file, 'r', encoding='utf-8') as f:
        prompt_content = f.read()

    client = OpenAI(
        api_key=API_KEY,
        base_url=API_URL,
        timeout=1800,
        max_retries=100,
    )

    messages = [
        {
            "role": "system",
            "content": "You are a helpful assistant specializing in mathematics and geometry."
        },
        {
            "role": "user",
            "content": prompt_content
        }
    ]

    id = 0
    first_round = True
    os.makedirs('responses', exist_ok=True)
    while True:

        if id == 0:
            first_round = False
            stream = client.responses.create(
                model=MODEL_NAME,
                input=messages,
                tools=tools,
                reasoning={"effort": "high", "summary": "detailed"},
                stream=True,
                timeout=1800
            )
        else:
            if first_round:
                with open(f'responses/response_{id}.json', 'r', encoding='utf-8') as f:
                    response = Response(**json.load(f))
                first_round = False
            resp_id = response.id
            messages = []
            have_function_call = False
            for output in response.output:
                if output.type != "function_call":
                    continue
                have_function_call = True
                call_id = output.call_id
                code = json.loads(output.arguments)['code']
                print(f"\n\nFunction call: {output.name}")
                print(code)
                func = function_map[output.name]
                result = func(code)
                print("\nFunction call result:", result)
                messages.append({
                    "type": "function_call_output",
                    "call_id": call_id,
                    "output": json.dumps(result)
                })
            if have_function_call:
                stream = client.responses.create(
                    model=MODEL_NAME,
                    input=messages,
                    tools=tools,
                    reasoning={"effort": "high", "summary": "detailed"},
                    stream=True,
                    timeout=1800,
                    previous_response_id=resp_id,
                )
            else:
                with open('responses/response.md', 'w', encoding='utf-8') as f:
                    f.write(response.output[1].content[0].text)
                break

        for chunk in stream:
            if chunk.type == 'response.reasoning_summary_part.added':
                print('\n\nReasoning:')
            elif chunk.type == 'response.reasoning_summary_text.delta':
                print(chunk.delta, end='')
            elif chunk.type == 'response.content_part.added':
                print('\n\nAnswer:')
            elif chunk.type == 'response.output_text.delta':
                print(chunk.delta, end='')
            elif chunk.type == 'response.completed':
                response = chunk.response

        id += 1
        with open(f'responses/response_{id}.json', 'w', encoding='utf-8') as f:
            f.write(response.model_dump_json(indent=2))
